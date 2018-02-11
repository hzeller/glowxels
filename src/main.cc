// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// (c) 2017 Henner Zeller <h.zeller@acm.org>
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

#include <errno.h>
#include <stdio.h>
#include <byteswap.h>
#include <strings.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include <stdint.h>
#include <unistd.h>

#include "image-processing.h"

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

// Data on SPI: MOSI:pin19, SCLK:23, CE0=Latch:24

#define ROW_LENGTH_BYTES (8*3)

// Our logical names
#define OUTPUT_ENABLE GPIO_18
#define MOTOR_ENABLE  GPIO_22
#define MOTOR_STEP    GPIO_24
#define MOTOR_DIR     GPIO_23

void motorSteps(int steps) {
    int udelay = 400;
    for (int i = 0; i < steps; ++i) {
        usleep(udelay);
        digitalWrite(MOTOR_STEP, LOW);
        usleep(udelay);
        digitalWrite(MOTOR_STEP, HIGH);
    }
}

void glowxels_bitmap(BitmapImage *img, bool dir) {
    const int byte_count = std::min(ROW_LENGTH_BYTES, img->width()/8);
    const int pixel_offset = 2;
    unsigned char *buffer = new unsigned char[byte_count];
    int light_steps = 30;   // We roughly need 37.8 steps per 1.5mm
    int dark_steps = 10;

    digitalWrite(MOTOR_ENABLE, LOW);
    digitalWrite(MOTOR_DIR, dir ? LOW : HIGH);

    for (int r = img->height() - pixel_offset - 1; r > 0; --r) {
        const uint8_t *first = img->GetRow(r);
        const uint8_t *second = img->GetRow(r + pixel_offset);
        for (int i = 0; i < byte_count; ++i) {
            buffer[i] = (*first++ & 0xaa) | (*second++ & 0x55);
        }
        wiringPiSPIDataRW(0, buffer, byte_count);
        digitalWrite(OUTPUT_ENABLE, LOW);
        motorSteps(light_steps);
        digitalWrite(OUTPUT_ENABLE, HIGH);
        motorSteps(dark_steps);
    }
    delete [] buffer;

    digitalWrite(MOTOR_ENABLE, HIGH);
}

int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options] <png-file>\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-d    : switch direction\n"
            "\t-i    : inverse image\n");
    return 1;
}

int main(int argc, char *argv[]) {
    bool dir = false;
    bool inverse = true;

    int opt;
    while ((opt = getopt(argc, argv, "di")) != -1) {
        switch (opt) {
        case 'd': dir = !dir; break;
        case 'i': inverse = !inverse; break;
        default:
            return usage(argv[0]);
        }
    }

    const char *filename = argv[optind];

    if (wiringPiSetup() < 0) {
        perror("Could not initialize wiringPi");
        return 1;
    }
    if (wiringPiSPISetup(0, 500000) < 0) {
        perror("Could not open SPI");
        return 1;
    }

    pinMode(OUTPUT_ENABLE, OUTPUT);  // for now, just output, but later PWM ?
    pinMode(MOTOR_ENABLE, OUTPUT);
    pinMode(MOTOR_STEP, OUTPUT);
    pinMode(MOTOR_DIR, OUTPUT);

    digitalWrite(MOTOR_ENABLE, HIGH);


    BitmapImage *img = LoadPNGImage(filename, inverse);
    if (!img) {
        fprintf(stderr, "Couldn't open %s\n", filename);
        return 1;
    }
    fprintf(stderr, "Showing %s %dx%d\n",
            filename, img->width(), img->height());
    glowxels_bitmap(img, dir);
    delete img;

    digitalWrite(MOTOR_ENABLE, HIGH);
    digitalWrite(OUTPUT_ENABLE, HIGH);
}
