#import "UIImage+RGBAData.h"

static void releaseCallback(void *info, const void *data, size_t size) {
  CFRelease((CFDataRef)info);
}

@implementation UIImage (RGBAData)

+ (UIImage *)imageWithRGBAData:(NSData *)data width:(size_t)width height:(size_t)height {
  size_t bytesPerPixel = 4;
  size_t bitsPerComponent = 8;
  size_t bitsPerPixel = bitsPerComponent * bytesPerPixel;
  size_t bytesPerRow = bytesPerPixel * width;
  CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault | kCGImageAlphaLast;

  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CFDataRef cfData = (__bridge_retained CFDataRef)data;
  CGDataProviderRef provider = CGDataProviderCreateWithData((void *)cfData,
                                                            data.bytes,
                                                            height * bytesPerRow,
                                                            releaseCallback);

  CGImageRef cgImage = CGImageCreate(width, height, bitsPerComponent, bitsPerPixel, bytesPerRow,
                                     colorSpace, bitmapInfo, provider,
                                     NULL, YES, kCGRenderingIntentDefault);

  UIImage *image = [UIImage imageWithCGImage:cgImage];

  CGColorSpaceRelease(colorSpace);
  CGDataProviderRelease(provider);
  CGImageRelease(cgImage);

  return image;
}

@end
