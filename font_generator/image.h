//
//  image.h
//  PRL_sdf_map
//
//  Created by roserg on 17/07/14.
//  Copyright (c) 2014 roserg. All rights reserved.
//

#ifndef __PRL_sdf_map__image__
#define __PRL_sdf_map__image__

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

using namespace std;

struct TGAHeader
{
    char Header[12];									// TGA File Header
};


struct TGA
{
    char		header[6];								// First 6 Useful Bytes From The Header
    unsigned int		bytesPerPixel;							// Holds Number Of Bytes Per Pixel Used In The TGA File
    unsigned int		imageSize;								// Used To Store The Image Size When Setting Aside Ram
    unsigned int		temp;									// Temporary Variable
    unsigned int		type;
    unsigned int		Height;									//Height of Image
    unsigned int		Width;									//Width ofImage
    unsigned int		Bpp;									// Bits Per Pixel
};

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

#endif /* defined(__PRL_sdf_map__image__) */
