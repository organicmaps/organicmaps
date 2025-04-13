#import "MWMImageCoder.h"

@implementation MWMImageCoder

- (UIImage *)imageWithData:(NSData *)data {
  UIImage *image = [UIImage imageWithData:data];
  if (!image) {
    return nil;
  }

  CGImageRef cgImage = image.CGImage;
  size_t width = CGImageGetWidth(cgImage);
  size_t height = CGImageGetHeight(cgImage);
  int32_t flags;
  if ([self imageHasAlpha:image]) {
    flags = kCGImageAlphaPremultipliedLast;
  } else {
    flags = kCGImageAlphaNoneSkipLast;
  }

  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGContextRef context = CGBitmapContextCreate(NULL, width, height, 8, width * 4, colorSpace, flags);
  CGColorSpaceRelease(colorSpace);

  CGContextDrawImage(context, CGRectMake(0, 0, width, height), cgImage);
  CGImageRef resultCgImage = CGBitmapContextCreateImage(context);
  UIImage *resultImage = [UIImage imageWithCGImage:resultCgImage];

  CGImageRelease(resultCgImage);
  CGContextRelease(context);

  return resultImage;
}

- (NSData *)dataFromImage:(UIImage *)image {
  if ([self imageHasAlpha:image]) {
    return UIImagePNGRepresentation(image);
  } else {
    return UIImageJPEGRepresentation(image, 0.9);
  }
}

- (BOOL)imageHasAlpha:(UIImage *)image {
  CGImageAlphaInfo alphaInfo = CGImageGetAlphaInfo(image.CGImage);
  return (alphaInfo == kCGImageAlphaPremultipliedLast ||
          alphaInfo == kCGImageAlphaPremultipliedFirst ||
          alphaInfo == kCGImageAlphaLast ||
          alphaInfo == kCGImageAlphaFirst);
}

@end
