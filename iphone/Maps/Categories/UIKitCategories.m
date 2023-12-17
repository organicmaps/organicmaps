#import "UIKitCategories.h"
#import "UIButton+RuntimeAttributes.h"
#import "UIImageView+Coloring.h"

#import <SafariServices/SafariServices.h>

@implementation NSObject (Optimized)

+ (NSString *)className { return NSStringFromClass(self); }
- (void)performAfterDelay:(NSTimeInterval)delayInSec block:(MWMVoidBlock)block
{
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSec * NSEC_PER_SEC)),
                 dispatch_get_main_queue(), ^{
                   block();
                 });
}

@end

@implementation UIView (Coordinates)

- (void)setMidX:(CGFloat)midX { self.center = CGPointMake(midX, self.center.y); }
- (CGFloat)midX { return self.center.x; }
- (void)setMidY:(CGFloat)midY { self.center = CGPointMake(self.center.x, midY); }
- (CGFloat)midY { return self.center.y; }
- (void)setOrigin:(CGPoint)origin
{
  self.frame = CGRectMake(origin.x, origin.y, self.frame.size.width, self.frame.size.height);
}

- (CGPoint)origin { return self.frame.origin; }
- (void)setMinX:(CGFloat)minX
{
  self.frame = CGRectMake(minX, self.frame.origin.y, self.frame.size.width, self.frame.size.height);
}

- (CGFloat)minX { return self.frame.origin.x; }
- (void)setMinY:(CGFloat)minY
{
  self.frame = CGRectMake(self.frame.origin.x, minY, self.frame.size.width, self.frame.size.height);
}

- (CGFloat)minY { return self.frame.origin.y; }
- (void)setMaxX:(CGFloat)maxX
{
  self.frame = CGRectMake(maxX - self.frame.size.width, self.frame.origin.y, self.frame.size.width,
                          self.frame.size.height);
}

- (CGFloat)maxX { return self.frame.origin.x + self.frame.size.width; }
- (void)setMaxY:(CGFloat)maxY
{
  self.frame = CGRectMake(self.frame.origin.x, maxY - self.frame.size.height, self.frame.size.width,
                          self.frame.size.height);
}

- (CGFloat)maxY { return self.frame.origin.y + self.frame.size.height; }
- (void)setWidth:(CGFloat)width
{
  self.frame = CGRectMake(self.frame.origin.x, self.frame.origin.y, width, self.frame.size.height);
}

- (CGFloat)width { return self.frame.size.width; }
- (void)setHeight:(CGFloat)height
{
  self.frame = CGRectMake(self.frame.origin.x, self.frame.origin.y, self.frame.size.width, height);
}

- (CGFloat)height { return self.frame.size.height; }
- (CGSize)size { return self.frame.size; }
- (void)setSize:(CGSize)size
{
  self.frame = CGRectMake(self.frame.origin.x, self.frame.origin.y, size.width, size.height);
}

- (void)sizeToIntegralFit
{
  [self sizeToFit];
  self.frame = CGRectIntegral(self.frame);
}

@end

@implementation UIApplication (URLs)

- (void)rateApp
{
  NSString * urlString = @"https://apps.apple.com/app/id1567437057?action=write-review";
  NSURL * url = [NSURL URLWithString:urlString];
  [self openURL:url options:@{} completionHandler:nil];
}

@end

@implementation SolidTouchView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {}
@end

@implementation UIView (Refresh)

@end

@implementation UITableViewCell (Refresh)

@end

@implementation UINavigationBar (Refresh)

@end

@implementation UILabel (Refresh)

@end

@implementation UISlider (Refresh)

@end

@implementation UISwitch (Refresh)

@end

@implementation UIButton (Refresh)

@end

@implementation UITextView (Refresh)

@end

@implementation UIImageView (Refresh)

@end

@implementation SolidTouchImageView

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {}
@end

@implementation UINavigationController (Autorotate)

- (BOOL)shouldAutorotate { return [self.viewControllers.lastObject shouldAutorotate]; }
@end

@implementation UIViewController (Autorotate)

- (NSUInteger)supportedInterfaceOrientations { return UIInterfaceOrientationMaskAll; }
@end

@interface UIViewController (SafariDelegateImpl)<SFSafariViewControllerDelegate>

@end

@implementation UIViewController (SafariDelegateImpl)

- (void)safariViewControllerDidFinish:(SFSafariViewController *)controller
{
  [self.navigationController dismissViewControllerAnimated:YES completion:nil];
}

@end

@implementation UIViewController (Safari)

- (BOOL)openUrl:(NSString * _Nonnull)urlString
{
  return [self openUrl:urlString inSafari:NO];
}

- (BOOL)openUrl:(NSString *)urlString inSafari:(BOOL)safari
{
  // TODO: This is a temporary workaround to open cyrillic/non-ASCII URLs.
  // URLs in OSM are stored in UTF-8. NSURL constructor documentation says:
  // > Must be a URL that conforms to RFC 2396. This method parses URLString according to RFCs 1738 and 1808.
  // The right way to encode the URL string should be:
  // 1. Split the (non-ASCII) string into components (host, path, query, fragment, etc.)
  // 2. Encode each component separately (they have different allowed characters).
  // 3. Merge them back into the string and create NSURL.
  NSMutableCharacterSet * charset = [[NSMutableCharacterSet alloc] init];
  [charset formUnionWithCharacterSet:NSCharacterSet.URLHostAllowedCharacterSet];
  [charset formUnionWithCharacterSet:NSCharacterSet.URLPathAllowedCharacterSet];
  [charset formUnionWithCharacterSet:NSCharacterSet.URLQueryAllowedCharacterSet];
  [charset formUnionWithCharacterSet:NSCharacterSet.URLFragmentAllowedCharacterSet];
  [charset addCharactersInString:@"#;/?:@&=+$,"];
  NSString * encoded = [urlString stringByAddingPercentEncodingWithAllowedCharacters:charset];
  // Matrix has an url with two hashes which doesn't work for NSURL and NSURLComponent.
  NSRange const matrixUrl = [encoded rangeOfString:@"#/#"];
  if (matrixUrl.location != NSNotFound)
    encoded = [encoded stringByReplacingOccurrencesOfString:@"#/#" withString:@"#/%23"];
  NSURLComponents * urlc = [NSURLComponents componentsWithString:encoded];
  if (!urlc)
  {
    NSAssert(false, @"Invalid URL %@", urlString);
    return NO;
  }
  // Some links in OSM do not have a scheme: www.some.link
  if (!urlc.scheme)
    urlc.scheme = @"http";

  NSURL * url = urlc.URL;
  if (safari)
  {
    [UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];
  }
  else
  {
    SFSafariViewController * svc = [[SFSafariViewController alloc] initWithURL:url];
    svc.delegate = self;
    [self.navigationController presentViewController:svc animated:YES completion:nil];
  }
  return YES;
}

@end

@implementation UIImage (ImageWithColor)

+ (UIImage *)imageWithColor:(UIColor *)color
{
  CGRect rect = CGRectMake(0.0, 0.0, 1.0, 1.0);
  UIGraphicsBeginImageContext(rect.size);
  CGContextRef context = UIGraphicsGetCurrentContext();

  CGContextSetFillColorWithColor(context, color.CGColor);
  CGContextFillRect(context, rect);

  UIImage * image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();

  return image;
}

@end
