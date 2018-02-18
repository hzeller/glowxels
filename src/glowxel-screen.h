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

#ifndef GLOW_GLOWXELS_SCREEN_H
#define GLOW_GLOWXELS_SCREEN_H

#include "image-processing.h"

class GlowxelScreen {
public:
    // Initialize hardware and return GlowxelScreen instance.
    // This is done as factory as it can fail initializing the hardware.
    // In that case, nullptr is returned.
    static GlowxelScreen *CreateInstance(bool reverse_stepper_direction);

    ~GlowxelScreen();

    // Set brightness level. Range value [0 .. 1.0]. Zero is 'off'.
    void SetBrightness(float b);

    // Set speed to roughly double the default speed, but with slightly
    // less quality.
    void SetFast(bool fast);

    // Retract to position before the last Eject().
    // The optional "extra_pixels" allow to scroll beyond that, e.g. if you
    // want to overwrite part of the last image.
    void Retract(int extra_pixels = 0);

    // Send an image to the screen. Multiple images can be send in a row.
    // For the last image to be visible, you need to call Eject().
    void ShowImage(const BitmapImage &img);

    // Scroll out the screen so that the last image sent is fully visible.
    // The optional "extra_pixels" allow to scroll beyond that.
    void Eject(int extra_pixels = 0);

private:
    // Initialize GlowxelScreen. Optionally allow to reverse direction
    // of stepper motor.
    // Don't call directly, call the factory.
    GlowxelScreen(bool reverse_stepper_direction);

    const bool stepper_direction_;
    float brightness_;
    int step_delay_;
};

#endif  // GLOW_GLOWXELS_SCREEN_H
