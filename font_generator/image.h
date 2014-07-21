#pragma once

// +----------------------------------------+
// |                                        |
// |   http://contourtextures.wikidot.com   |
// |                                        |
// +----------------------------------------+

/*
Copyright (C) 2009 by Stefan Gustavson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

using namespace std;

namespace
{
struct TGAHeader
{
  char Header[12];
};

struct TGA
{
  char		header[6];
  unsigned int		bytesPerPixel;
  unsigned int		imageSize;
  unsigned int		temp;
  unsigned int		type;
  unsigned int		Height;
  unsigned int		Width;
  unsigned int		Bpp;
};
}

class image
{
public:
  int height;
  int width;
  float *data;
public:
  image(int H, int W);
  image(int H, int W, float *arr);
  image(int H, int W, unsigned char *arr);
  image(){}
  image(const image &copy);
  image(string path);
  ~image();
  void write_as_tga(string path);
  void computegradient(image &grad_x, image  &grad_y);
  void scale();
  void invert();
  float distaa3(image &gximg, image &gyimg, int w, int c, int xc, int yc, int xi, int yi);
  void edtaa3(image &gx, image &gy, int w, int h, short *distx, short *disty, image &dist);
  void minus(image &im);
  void distquant();
  void to_uint8_t_vec(vector<uint8_t> &dst);
  image bilinear(float scale);
  image generate_SDF(float sc);
  image add_border(int size);
};

