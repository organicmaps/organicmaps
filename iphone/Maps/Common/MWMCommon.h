#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MWMConsts.h"

static inline BOOL firstVersionIsLessThanSecond(NSString * first, NSString * second)
{
  NSArray const * const f = [first componentsSeparatedByString:@"."];
  NSArray const * const s = [second componentsSeparatedByString:@"."];
  NSUInteger iter = 0;
  while (f.count > iter && s.count > iter)
  {
    NSInteger fiv = ((NSString *)f[iter]).integerValue;
    NSInteger siv = ((NSString *)s[iter]).integerValue;

    if (fiv == siv)
      iter++;
    else
      return fiv < siv;
  }
  return f.count < s.count;
}

static inline BOOL isIOSVersionLessThan(NSString * version)
{
  return firstVersionIsLessThanSecond([UIDevice currentDevice].systemVersion, version);
}

static inline BOOL isIOSVersionLessThan(NSUInteger version)
{
  return isIOSVersionLessThan([NSString stringWithFormat:@"%@", @(version)]);
}

static BOOL const isIOS8 = isIOSVersionLessThan(9);

static inline BOOL isInterfaceRightToLeft()
{
  return [UIApplication sharedApplication].userInterfaceLayoutDirection ==
         UIUserInterfaceLayoutDirectionRightToLeft;
}

static inline NSString * formattedSize(uint64_t size)
{
  return
      [NSByteCountFormatter stringFromByteCount:size countStyle:NSByteCountFormatterCountStyleFile];
}

// Use only for screen dimensions CGFloat comparison
static inline BOOL equalScreenDimensions(CGFloat left, CGFloat right)
{
  return fabs(left - right) < 0.5;
}

static inline CGFloat statusBarHeight()
{
  CGSize const statusBarSize = [UIApplication sharedApplication].statusBarFrame.size;
  return MIN(statusBarSize.height, statusBarSize.width);
}

static inline void runAsyncOnMainQueue(dispatch_block_t block)
{
  dispatch_async(dispatch_get_main_queue(), block);
}

static inline void setStatusBarBackgroundColor(UIColor * color)
{
  UIView * statusBar =
      [[UIApplication sharedApplication] valueForKeyPath:@"statusBarWindow.statusBar"];
  if ([statusBar respondsToSelector:@selector(setBackgroundColor:)])
    statusBar.backgroundColor = color;
}
