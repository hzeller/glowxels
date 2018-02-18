// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// (c) 2017 Henner Zeller <h.zeller@acm.org>
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

#include <getopt.h>

#include <memory>

#include "image-processing.h"
#include "glowxel-screen.h"

int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options] <png-file> [<png-file>...]\n",
            progname);
    fprintf(stderr, "Options:\n"
            "\t-i    : inverse image\n"
            "\t-r    : Retract to last write position before showing image.\n"
            "\t-F    : fast (slightly lower brightness + quality)\n"
            "\t-b<%%> : Brightness percent. Default 70.\n"
            );
    return 1;
}

int main(int argc, char *argv[]) {
    bool dir = false;
    bool inverse = true;
    bool retract = false;
    bool fast = false;
    float brightness = 0.7;

    int opt;
    while ((opt = getopt(argc, argv, "dirb:F")) != -1) {
        switch (opt) {
        case 'd': dir = !dir; break;  // In case the motor is wired backwards.
        case 'i': inverse = !inverse; break;
        case 'r': retract = true; break;
        case 'b': brightness = atof(optarg) / 100.0; break;
        case 'F': fast = true; break;
        default:
            return usage(argv[0]);
        }
    }

    if (brightness < 0 || brightness > 1.0) {
        fprintf(stderr, "Brightness needs to be betwen 1..100%%\n");
        return usage(argv[0]);
    }

    if (optind >= argc)
        return usage(argv[0]);


    std::unique_ptr<GlowxelScreen> screen(GlowxelScreen::CreateInstance(dir));
    if (!screen) {
        fprintf(stderr, "Could not initialize hardware\n");
        return 1;
    }

    screen->SetFast(fast);
    screen->SetBrightness(brightness);

    if (retract) screen->Retract();

    for (int imgarg = optind; imgarg < argc; ++imgarg) {
        const char *filename = argv[imgarg];
        BitmapImage *img = LoadPNGImage(filename, inverse);
        if (!img) {
            fprintf(stderr, "Couldn't open %s\n", filename);
            continue;
        }
        fprintf(stderr, "Showing %s %dx%d\n",
                filename, img->width(), img->height());
        screen->ShowImage(*img);
        delete img;
    }

    screen->Eject();
}
