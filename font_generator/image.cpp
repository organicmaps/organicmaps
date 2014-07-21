//
//  image.cpp
//  PRL_sdf_map
//
//  Created by roserg on 17/07/14.
//  Copyright (c) 2014 roserg. All rights reserved.
//

#include "image.h"

#define DISTAA(c,xc,yc,xi,yi) (distaa3(gx, gy, w, c, xc, yc, xi, yi))

using namespace std;

void mexFunction(image &img, image &out)
{
    image gx(img.height, img.width);
    image gy(img.height, img.width);
    short *xdist, *ydist;
    int mrows,ncols;
    int i;

    /* The input must be a noncomplex, double 2-D array. */
    mrows = img.width;
    ncols = img.height;


    /* Call the C subroutine. */
    xdist = (short*)malloc(mrows*ncols*sizeof(short)); // local data
    ydist = (short*)malloc(mrows*ncols*sizeof(short));
    img.computegradient(gx, gy);
    img.edtaa3(gx, gy, mrows, ncols, xdist, ydist, out);
    // Pixels with grayscale>0.5 will have a negative distance.
    // This is correct, but we don't want values <0 returned here.
    for(i=0; i<mrows*ncols; i++)
    {
        if(out.data[i] < 0)
            out.data[i]=0.0;
    }

    //    for(i=0; i<mrows*ncols; i++)
    //    {
    //        in[i] = i+1 - xdist[i] - ydist[i]*mrows;
    //    }

    free(xdist); // Local data allocated with mxMalloc()
    free(ydist); // (Would be automatically deallocated, but be tidy)

}

double edgedf(double gx, double gy, double a)
{
    double df, glength, temp, a1;

    if ((gx == 0) || (gy == 0)) { // Either A) gu or gv are zero, or B) both
        df = 0.5-a;  // Linear approximation is A) correct or B) a fair guess
    } else {
        glength = sqrt(gx*gx + gy*gy);
        if(glength>0) {
            gx = gx/glength;
            gy = gy/glength;
        }
        /* Everything is symmetric wrt sign and transposition,
         * so move to first octant (gx>=0, gy>=0, gx>=gy) to
         * avoid handling all possible edge directions.
         */
        gx = fabs(gx);
        gy = fabs(gy);
        if(gx<gy) {
            temp = gx;
            gx = gy;
            gy = temp;
        }
        a1 = 0.5*gy/gx;
        if (a < a1) { // 0 <= a < a1
            df = 0.5*(gx + gy) - sqrt(2.0*gx*gy*a);
        } else if (a < (1.0-a1)) { // a1 <= a <= 1-a1
            df = (0.5-a)*gx;
        } else { // 1-a1 < a <= 1
            df = -0.5*(gx + gy) + sqrt(2.0*gx*gy*(1.0-a));
        }
    }
    return df;
}

image::image(int H, int W)
{
    height = H;
    width = W;
    data = new float[height * width];
    for(int i = 0 ; i < height * width ; ++i)
    {
        data[i] = 0.0f;
    }
}

image::image(int H, int W, float *arr)
{
    height = H;
    width = W;
    data = new float[height * width];
    for(int i = 0 ; i < height * width ; ++i)
    {
        data[i] = arr[i];
    }
}

image::image(int H, int W, unsigned char *arr)
{
    height = H;
    width = W;
    data = new float[height * width];
    for(int i = 0 ; i < height * width ; ++i)
    {
        data[i] = (float)arr[i]/255.0f;
    }
}

image::image(const image &copy)
{
    height = copy.height;
    width = copy.width;
    data = new float[height * width];
    for(int i = 0 ; i < height * width ; ++i)
    {
        data[i] = copy.data[i];
    }
}

void image::to_uint8_t_vec(vector<uint8_t> &dst)
{
    for(int i = 0 ; i < height * width ; ++i)
    {
        dst[i] = data[i]*255.0f;
    }
}

image::~image()
{
    if(data != NULL)
        delete [] data;
}

image::image(string path)
{

}

void image::write_as_tga(string path)
{
    FILE *sFile = 0;

    unsigned char tgaHeader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned char header[6];
    unsigned char bits = 0;
    int colorMode = 0;

    sFile = fopen(path.c_str(), "wb");

    if(!sFile)
    {
        printf("Can't open ScreenShot File! Error\n");
        return;
    }

    colorMode = 3;
    bits = 24;

    header[0] = width % 256;
    header[1] = width / 256;
    header[2] = height % 256;
    header[3] = height / 256;
    header[4] = bits;
    header[5] = 0;

    fwrite(tgaHeader, sizeof(tgaHeader), 1, sFile);
    fwrite(header, sizeof(header), 1, sFile);

    char *data2 = new char[width * height *3];
    for(int i = 0; i < width * height; ++i)
    {
        data2[3*i] = data[i] * 255.0f;
        data2[3*i+1] = data[i] * 255.0f;
        data2[3*i+2] = data[i] * 255.0f;
    }

    fwrite(data2, width * height * colorMode, 1, sFile);

    delete [] data2;

    fclose(sFile);
}

void image::computegradient(image &grad_x, image  &grad_y)
{
    int i,j,k;
    float glength;
    const float SQRT2 = 1.4142136f;
    for(i = 1; i < height-1; i++) // Avoid edges where the kernels would spill over
    {
        for(j = 1; j < width-1; j++)
        {
            k = i*width + j;
            if((data[k]>0.0) && (data[k]<1.0)) // Compute gradient for edge pixels only
            {
                grad_x.data[k] = -(data[k-width-1] + SQRT2*data[k-1] + data[k+width-1]) +
                (data[k-width+1] + SQRT2*data[k+1] + data[k+width+1]);
                grad_y.data[k] = -(data[k-width-1] + SQRT2*data[k-width] + data[k+width-1]) +
                (data[k-width+1] + SQRT2*data[k+width] + data[k+width+1]);
                glength = grad_x.data[k]*grad_x.data[k] + grad_y.data[k]*grad_y.data[k];
                if(glength > 0.0) // Avoid division by zero
                {
                    glength = sqrt(glength);
                    grad_x.data[k] /= glength;
                    grad_y.data[k] /= glength;
                }
            }
        }
    }
}

void image::scale()
{
    float maxi = -1000, mini = 1000;
    for(int i = 0; i < width * height; ++i)
    {
        maxi = max(maxi, data[i]);
        mini = min(mini, data[i]);
    }

    for(int i = 0; i < width * height; ++i)
    {
        data[i] -= mini;
    }
    maxi -= mini;
    for(int i = 0; i < width * height; ++i)
    {
        data[i] /= maxi;
    }
}

void image::invert()
{
    for(int i = 0; i < width * height; ++i)
    {
        data[i] = 1.0f - data[i];
    }
}

void image::minus(image &im)
{
    for(int i = 0; i < width * height; ++i)
    {
        data[i] -= im.data[i];
    }
}

void image::distquant()
{
    for(int i = 0; i < width * height; ++i)
    {
        float k = 0.0325f;
        data[i] = 0.5f + data[i]*k;
        data[i] = max(0.0f, data[i]);
        data[i] = min(1.0f, data[i]);
    }
}

image image::add_border(int size)
{
  image res(height + 2.0 * size, width + 2.0 * size);
  for(int i = 0; i < size; ++i)
  {
    int index = i*res.width;
    for(int j = 0; j < res.width ; ++j)
    {
      res.data[index + j] = 0;
    }
  }
  for(int i = height + size; i < height + 2*size; ++i)
  {
    int index = i*res.width;
    for(int j = 0; j < res.width ; ++j)
    {
      res.data[index + j] = 0;
    }
  }
  for(int i = size; i < res.height - size; ++i)
  {
    int index = i*res.width;
    int index2 = (i-size)*width;
    for(int j = 0; j < size ; ++j)
    {
      res.data[index + j] = 0;
    }
    for(int j = size ; j < width+size ; ++j)
    {
      res.data[index + j] = data[index2 + j - size];
    }
    for(int j = width+size; j < width + 2*size ; ++j)
    {
      res.data[index + j] = 0;
    }
  }

  return res;
}

image image::generate_SDF(float sc)
{
    scale();

    image inv(*this);
    inv.invert();

    image outside(height, width);
    image inside(height, width);
    mexFunction(*this, outside);
    mexFunction(inv, inside);

    outside.minus(inside);

    outside.distquant();
    outside.invert();

    return outside.bilinear(sc);
}

image image::bilinear(float scale)
{
    image result(height*scale, width*scale);
    int x, y, index;
    float A, B, C, D, gray;
    int w2 = result.width;
    int h2 = result.height;
    float x_ratio = ((float)(width-1))/w2 ;
    float y_ratio = ((float)(height-1))/h2 ;
    float x_diff, y_diff;
    int offset = 0 ;
    for (int i=0;i<h2;i++)
    {
        for (int j=0;j<w2;j++)
        {
            x = (int)(x_ratio * j) ;
            y = (int)(y_ratio * i) ;
            x_diff = (x_ratio * j) - x ;
            y_diff = (y_ratio * i) - y ;
            index = y*width+x ;

            // range is 0 to 255 thus bitwise AND with 0xff
            A = data[index];
            B = data[index+1];
            C = data[index+width];
            D = data[index+width+1];

            // Y = A(1-w)(1-h) + B(w)(1-h) + C(h)(1-w) + Dwh
            gray = A*(1.0f-x_diff)*(1.0f-y_diff) +  B*(x_diff)*(1.0f-y_diff) +
                         C*(y_diff)*(1.0f-x_diff)   +  D*(x_diff*y_diff);

            result.data[offset++] = gray;
        }
    }
    return result ;
}

float image::distaa3(image &gximg, image &gyimg, int w, int c, int xc, int yc, int xi, int yi)
{
    double di, df, dx, dy, gx, gy, a;
    int closest;

    closest = c-xc-yc*w; // Index to the edge pixel pointed to from c
    a = data[closest];    // Grayscale value at the edge pixel
    gx = gximg.data[closest]; // X gradient component at the edge pixel
    gy = gyimg.data[closest]; // Y gradient component at the edge pixel

    if(a > 1.0) a = 1.0;
    if(a < 0.0) a = 0.0; // Clip grayscale values outside the range [0,1]
    if(a == 0.0) return 1000000.0; // Not an object pixel, return "very far" ("don't know yet")

    dx = (double)xi;
    dy = (double)yi;
    di = sqrt(dx*dx + dy*dy); // Length of integer vector, like a traditional EDT
    if(di==0) { // Use local gradient only at edges
        // Estimate based on local gradient only
        df = edgedf(gx, gy, a);
    } else {
        // Estimate gradient based on direction to edge (accurate for large di)
        df = edgedf(dx, dy, a);
    }
    return di + df; // Same metric as edtaa2, except at edges (where di=0)
}

void image::edtaa3(image &gx, image &gy, int w, int h, short *distx, short *disty, image &dist)
{
    int x, y, i, c;
    int offset_u, offset_ur, offset_r, offset_rd,
    offset_d, offset_dl, offset_l, offset_lu;
    double olddist, newdist;
    int cdistx, cdisty, newdistx, newdisty;
    int changed;
    double epsilon = 1e-3;

    /* Initialize index offsets for the current image width */
    offset_u = -w;
    offset_ur = -w+1;
    offset_r = 1;
    offset_rd = w+1;
    offset_d = w;
    offset_dl = w-1;
    offset_l = -1;
    offset_lu = -w-1;

    /* Initialize the distance images */
    for(i=0; i<w*h; i++) {
        distx[i] = 0; // At first, all pixels point to
        disty[i] = 0; // themselves as the closest known.
        if(data[i] <= 0.0)
        {
            dist.data[i]= 1000000.0; // Big value, means "not set yet"
        }
        else if (data[i]<1.0) {
            dist.data[i] = edgedf(gx.data[i], gy.data[i], data[i]); // Gradient-assisted estimate
        }
        else {
            dist.data[i]= 0.0; // Inside the object
        }
    }

    /* Perform the transformation */
    do
    {
        changed = 0;

        /* Scan rows, except first row */
        for(y=1; y<h; y++)
        {

            /* move index to leftmost pixel of current row */
            i = y*w;

            /* scan right, propagate distances from above & left */

            /* Leftmost pixel is special, has no left neighbors */
            olddist = dist.data[i];
            if(olddist > 0) // If non-zero distance or not set yet
            {
                c = i + offset_u; // Index of candidate for testing
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }

                c = i+offset_ur;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    changed = 1;
                }
            }
            i++;

            /* Middle pixels have all neighbors */
            for(x=1; x<w-1; x++, i++)
            {
                olddist = dist.data[i];
                if(olddist <= 0) continue; // No need to update further

                c = i+offset_l;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }

                c = i+offset_lu;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }

                c = i+offset_u;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }

                c = i+offset_ur;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    changed = 1;
                }
            }

            /* Rightmost pixel of row is special, has no right neighbors */
            olddist = dist.data[i];
            if(olddist > 0) // If not already zero distance
            {
                c = i+offset_l;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }

                c = i+offset_lu;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }

                c = i+offset_u;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx;
                newdisty = cdisty+1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    changed = 1;
                }
            }

            /* Move index to second rightmost pixel of current row. */
            /* Rightmost pixel is skipped, it has no right neighbor. */
            i = y*w + w-2;

            /* scan left, propagate distance from right */
            for(x=w-2; x>=0; x--, i--)
            {
                olddist = dist.data[i];
                if(olddist <= 0) continue; // Already zero distance

                c = i+offset_r;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    changed = 1;
                }
            }
        }

        /* Scan rows in reverse order, except last row */
        for(y=h-2; y>=0; y--)
        {
            /* move index to rightmost pixel of current row */
            i = y*w + w-1;

            /* Scan left, propagate distances from below & right */

            /* Rightmost pixel is special, has no right neighbors */
            olddist = dist.data[i];
            if(olddist > 0) // If not already zero distance
            {
                c = i+offset_d;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }

                c = i+offset_dl;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    changed = 1;
                }
            }
            i--;

            /* Middle pixels have all neighbors */
            for(x=w-2; x>0; x--, i--)
            {
                olddist = dist.data[i];
                if(olddist <= 0) continue; // Already zero distance

                c = i+offset_r;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }

                c = i+offset_rd;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }

                c = i+offset_d;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }

                c = i+offset_dl;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    changed = 1;
                }
            }
            /* Leftmost pixel is special, has no left neighbors */
            olddist = dist.data[i];
            if(olddist > 0) // If not already zero distance
            {
                c = i+offset_r;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }

                c = i+offset_rd;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx-1;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    olddist=newdist;
                    changed = 1;
                }

                c = i+offset_d;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx;
                newdisty = cdisty-1;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    changed = 1;
                }
            }

            /* Move index to second leftmost pixel of current row. */
            /* Leftmost pixel is skipped, it has no left neighbor. */
            i = y*w + 1;
            for(x=1; x<w; x++, i++)
            {
                /* scan right, propagate distance from left */
                olddist = dist.data[i];
                if(olddist <= 0) continue; // Already zero distance

                c = i+offset_l;
                cdistx = distx[c];
                cdisty = disty[c];
                newdistx = cdistx+1;
                newdisty = cdisty;
                newdist = DISTAA(c, cdistx, cdisty, newdistx, newdisty);
                if(newdist < olddist-epsilon)
                {
                    distx[i]=newdistx;
                    disty[i]=newdisty;
                    dist.data[i]=newdist;
                    changed = 1;
                }
            }
        }
    }
    while(changed); // Sweep until no more updates are made

    /* The transformation is completed. */

}
