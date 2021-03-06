// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// (c) 2018 Henner Zeller <h.zeller@acm.org>
//
// This file is part of Gloxels. http://glowxels.net/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>

#include "glowxel-screen.h"

#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

/*
 * Here are some hardcoded constants matching the
 * one instance in existence :)
 */

// Our screen has 320 pixels accross.
static constexpr int kRowLengthBytes = 320 / 8;

// Stepmotor steps with each pixel. This is specific to the mounting
// set-up, diameter of roller etc.
static constexpr int kStepsPerPixel = 36;
static constexpr int kCanvasEjectSteps = 50 * kStepsPerPixel;

// Steps we use to accelerate and decelerate. We're not doing any kind of
// trapezoidal acceleration, just jump from zero to half-speed
// for kAccelerateSteps before we do full speed. Good enough.
static constexpr int kAccelerateSteps = 20 * kStepsPerPixel;

static constexpr int kRegularStepDelay = 200;
static constexpr int kFastStepDelay = 100;

/* the wiringPi libray has some WTF random assignments to what it calls
 * Pins, but they have nothing to do with the GPIO pins.
 * http://wiringpi.com/pins/special-pin-functions/
 *
 * Bring back some sanity.
 */
enum GPIO_NAMES {
    GPIO_18 = 1,  // header 12
    GPIO_22 = 3,  // header 15
    GPIO_23 = 4,  // header 16
    GPIO_24 = 5   // header 18
};

// Data on SPI: MOSI:header19, SCLK:header23, CE0=Latch:header24

// Our logical names
#define OUTPUT_ENABLE GPIO_18
#define MOTOR_ENABLE  GPIO_22
#define MOTOR_STEP    GPIO_24
#define MOTOR_DIR     GPIO_23

GlowxelScreen *GlowxelScreen::CreateInstance(bool reverse_stepper_direction) {
    if (wiringPiSetup() < 0) {
        perror("Could not initialize wiringPi");
        return nullptr;
    }
    if (wiringPiSPISetup(0, 500000) < 0) {
        perror("Could not open SPI");
        return nullptr;
    }

    return new GlowxelScreen(reverse_stepper_direction);
}

GlowxelScreen::GlowxelScreen(bool reverse_stepper_direction)
    : stepper_direction_wiring_(reverse_stepper_direction),
      brightness_(0.7), step_delay_(kRegularStepDelay),
      motor_is_on_(false), motor_direction_(false) {
    pinMode(OUTPUT_ENABLE, OUTPUT);     // Maybe later PWM ?
    pinMode(MOTOR_ENABLE, OUTPUT);
    pinMode(MOTOR_STEP, OUTPUT);
    pinMode(MOTOR_DIR, OUTPUT);
}

GlowxelScreen::~GlowxelScreen() {
    digitalWrite(OUTPUT_ENABLE, HIGH);  // Should be turned off, but extra sure.
    digitalWrite(MOTOR_ENABLE, HIGH);   // Set motor to coasting again.
}

void GlowxelScreen::SetBrightness(float b) {
    if (b >= 0.0 && b <= 1.0) brightness_ = b;
}

void GlowxelScreen::SetFast(bool fast) {
    step_delay_ = fast ? kFastStepDelay : kRegularStepDelay;
}

static void motorSteps(int steps, int step_delay) {
    for (int i = 0; i < steps; ++i) {
        // For our application, usleep() is good enough; no realtimeness needed
        usleep(step_delay);
        digitalWrite(MOTOR_STEP, LOW);
        usleep(step_delay);
        digitalWrite(MOTOR_STEP, HIGH);
    }
}

bool GlowxelScreen::SetMotorState(bool on, bool dir) {
    dir ^= stepper_direction_wiring_;
    if (motor_is_on_ == on && dir == motor_direction_)
        return false;
    digitalWrite(MOTOR_DIR, dir ? LOW : HIGH);
    motor_direction_ = dir;
    digitalWrite(MOTOR_ENABLE, on ? LOW : HIGH);
    motor_is_on_ = on;
    return true;
}

// Get row if it is within range or return fallback buffer if not.
static const uint8_t *RowOrFallback(const BitmapImage &img, int row,
                                    const uint8_t *fallback) {
    if (row < 0 || row >= img.height()) return fallback;
    return img.GetRow(row);
}

void GlowxelScreen::ShowImage(const BitmapImage &img) {
    if (SetMotorState(true, true)) {
        // Need to do some acceleration first.
        motorSteps(kAccelerateSteps, 2*step_delay_);
    }

    const int image_width_bytes = std::min(img.width() / 8, kRowLengthBytes);
    const int y_pixel_offset = 3;  // LEDs are mounted zigzag offset with 3.
    unsigned char *const buffer = new unsigned char[kRowLengthBytes];
    const int start_byte = kRowLengthBytes - image_width_bytes;
    const int light_steps = std::max(0.0f, brightness_ * kStepsPerPixel);
    const int dark_steps = kStepsPerPixel - light_steps;

    for (int r = img.height() - 1; r >= -y_pixel_offset; --r) {
        memset(buffer, 0x00, kRowLengthBytes);
        // If outside pixel range, we just alias with our empty buffer
        const uint8_t *first  = RowOrFallback(img, r, buffer);
        const uint8_t *second = RowOrFallback(img, r + y_pixel_offset, buffer);
        for (int i = 0; i < image_width_bytes; ++i) {
            // Alternative bits from alternative rows to match zig-zag LED mount
            buffer[start_byte+i] = (*first++ & 0x55) | (*second++ & 0xaa);
        }
        wiringPiSPIDataRW(0, buffer, kRowLengthBytes);
        digitalWrite(OUTPUT_ENABLE, LOW);
        motorSteps(light_steps, step_delay_);
        digitalWrite(OUTPUT_ENABLE, HIGH);
        motorSteps(dark_steps, step_delay_);
    }
    delete [] buffer;
}

void GlowxelScreen::Retract(int extra_pixels) {
    SetMotorState(true, false);
    motorSteps(kAccelerateSteps, 2*step_delay_);
    motorSteps(kCanvasEjectSteps - kAccelerateSteps, step_delay_);
    // decelerate, and prepare for the acceleration
    motorSteps(kAccelerateSteps + extra_pixels*kStepsPerPixel, 2*step_delay_);
}

void GlowxelScreen::Eject(int extra_pixels) {
    motorSteps(kCanvasEjectSteps - kAccelerateSteps, step_delay_);
    motorSteps(kAccelerateSteps + extra_pixels*kStepsPerPixel, 2*step_delay_);
    SetMotorState(false, true);
}
