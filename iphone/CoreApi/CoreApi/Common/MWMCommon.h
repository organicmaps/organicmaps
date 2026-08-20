#import <UIKit/UIKit.h>

#import "MWMTypes.h"

NS_ASSUME_NONNULL_BEGIN

static inline BOOL firstVersionIsLessThanSecond(NSString * first, NSString * second)
{
  NSArray<NSString *> * f = [first componentsSeparatedByString:@"."];
  NSArray<NSString *> * s = [second componentsSeparatedByString:@"."];
  NSUInteger iter = 0;
  while (f.count > iter && s.count > iter)
  {
    NSInteger fiv = f[iter].integerValue;
    NSInteger siv = s[iter].integerValue;

    if (fiv == siv)
      iter++;
    else
      return fiv < siv;
  }
  return f.count < s.count;
}

static inline BOOL isInterfaceRightToLeft(void) NS_EXTENSION_UNAVAILABLE_IOS("Not available in extensions")
{
  return UIApplication.sharedApplication.userInterfaceLayoutDirection == UIUserInterfaceLayoutDirectionRightToLeft;
}

static inline NSString * formattedSize(uint64_t size)
{
  return [NSByteCountFormatter stringFromByteCount:size countStyle:NSByteCountFormatterCountStyleFile];
}

NS_ASSUME_NONNULL_END
