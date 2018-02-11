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

#include "image-processing.h"
#include <png.h>
#include <string.h>
#include <math.h>

#include <functional>

void BitmapImage::ToPBM(FILE *file) const {
    fprintf(file, "P4\n%d %d\n", width_, height_);
    fwrite(bits_->buffer(), 1, width_ * height_ / 8, file);
    fclose(file);
}

bool BitmapImage::CopyFrom(const BitmapImage &other) {
    if (other.width_ != width_ || other.height_ != height_) return false;
    memcpy(bits_->buffer(), other.bits_->buffer(), bits_->size_bits() / 8);
    return true;
}

BitmapImage *LoadPNGImage(const char *filename, bool invert) {
    //  More or less textbook libpng tutorial.
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Opening image");
        return NULL;
    }
    const png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                   NULL, NULL, NULL);
    if (!png) return NULL;

    const png_infop info = png_create_info_struct(png);
    if (!info) return NULL;

    if (setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "Issue reading %s\n", filename);
        return NULL;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    const png_byte bit_depth = png_get_bit_depth(png, info);
    if (bit_depth == 16)
        png_set_strip_16(png);

    const png_byte color_type = png_get_color_type(png, info);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    // So, for anyone who knows the libpng: is there no way to get a bitmap
    // out of it directly ?
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_rgb_to_gray(png, 1 /*PNG_ERROR_ACTION_NONE*/,
                            -1 /*PNG_RGB_TO_GRAY_DEFAULT*/,
                            -1 /*PNG_RGB_TO_GRAY_DEFAULT*/);
    }

    const int width = png_get_image_width(png, info);
    const int height = png_get_image_height(png, info);

    png_uint_32 res_x = 0, res_y = 0;
    int unit;
    png_get_pHYs(png, info, &res_x, &res_y, &unit);
    //*dpi = (unit == PNG_RESOLUTION_METER) ? res_x / (1000 / 25.4) : res_x;

    png_read_update_info(png, info);

    // Let's make sure our image is already multiple to 8x8 pixels and round
    // up the values is needed. That way, we can rotate without reading from
    // invalid locations. Width is already internally rounded to the next byte,
    // we only need to make sure height properly rounded.
    BitmapImage *result = new BitmapImage(width, (height + 7) & ~0x7);

    const int bytes_per_pixel = png_get_rowbytes(png, info) / width;

    // Our BitmapImage rounds up to the next full byte, so make sure that
    // we allocate potential free space beyond what png would write.
    png_byte *const row_data = new png_byte[bytes_per_pixel * result->width()]();

    //fprintf(stderr, "Reading %dx%d image (res=%.f).\n", width, height, *dpi);
    for (int y = 0; y < height; ++y) {
        png_read_row(png, row_data, NULL);
        png_byte *from_pixel = row_data;
        uint8_t *to_byte = result->GetMutableRow(y);
        for (int x = 0; x < result->width(); x+= 8) {
            *to_byte |= (*from_pixel > 128) << 7; from_pixel += bytes_per_pixel;
            *to_byte |= (*from_pixel > 128) << 6; from_pixel += bytes_per_pixel;
            *to_byte |= (*from_pixel > 128) << 5; from_pixel += bytes_per_pixel;
            *to_byte |= (*from_pixel > 128) << 4; from_pixel += bytes_per_pixel;
            *to_byte |= (*from_pixel > 128) << 3; from_pixel += bytes_per_pixel;
            *to_byte |= (*from_pixel > 128) << 2; from_pixel += bytes_per_pixel;
            *to_byte |= (*from_pixel > 128) << 1; from_pixel += bytes_per_pixel;
            *to_byte |= (*from_pixel > 128) << 0; from_pixel += bytes_per_pixel;
            if (invert) *to_byte = ~*to_byte;
            to_byte += 1;
        }
    }

    delete [] row_data;

    fclose(fp);
    return result;
}
