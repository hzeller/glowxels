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

#ifndef GLOW_IMAGE_PROCESSING_H
#define GLOW_IMAGE_PROCESSING_H

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include <vector>

class BitArray {
public:
    explicit BitArray(size_t size)
        : size_(size), buffer_(new uint8_t[(size_+7)/8]) {
        Clear();
    }
    BitArray(const BitArray &other)
        : size_(other.size_), buffer_(new uint8_t[(size_+7)/8]) {
        memcpy(buffer_, other.buffer_, (size_+7)/8);
    }
    ~BitArray() { delete [] buffer_; }

    void Clear() {  bzero(buffer_, (size_+7)/8); }

    inline void Set(int bit, bool value) {
        if (bit < 0 || bit >= size_) return;
        if (value)
            buffer_[bit / 8] |= 1 << (7 - bit % 8);
        else
            buffer_[bit / 8] &= ~(1 << (7 - bit % 8));
    }
    inline bool Get(int bit) const {
        return buffer_[bit / 8] & (1 << (7 - bit % 8));
    }

    uint8_t *buffer() { return buffer_; }
    int size_bits() const { return size_; }

private:
    const int size_;
    uint8_t *const buffer_;
};

// A bitmap image with packed bits and direct access.
// Image width is aligned to the next full byte.
class BitmapImage {
public:
    BitmapImage(int width, int height)
        : width_((width + 7) & ~0x7), height_(height),
          bits_(new BitArray(width_ * height)) {}
    BitmapImage(const BitmapImage &o)
        : width_(o.width_), height_(o.height_), bits_(new BitArray(*o.bits_)) {
    }

    ~BitmapImage() { delete bits_; }

    int width() const { return width_; }
    int height() const { return height_; }

    inline bool Get(int x, int y) const {
        assert(x >= 0 && x < width_ && y >= 0 && y < height_);
        return bits_->Get(width_ * y + x);
    }
    inline void Set(int x, int y, bool value) {
        assert(x >= 0 && x < width_ && y >= 0 && y < height_);
        bits_->Set(width_ * y + x, value);
    }

    // Raw read access to a full row.
    const uint8_t *GetRow(int r) const {
        return bits_->buffer() + r * width_ / 8;
    }
    uint8_t *GetMutableRow(int r) { return bits_->buffer() + r * width_ / 8; }

    bool CopyFrom(const BitmapImage &other);
    void ToPBM(FILE *file) const;

private:
    const int width_, height_;
    BitArray *const bits_;
};

// Load PNG file, convert to grayscale and return result as allocated
// SimpleImage. NULL on failure.
BitmapImage *LoadPNGImage(const char *filename, bool invert);

#endif
