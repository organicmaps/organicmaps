#import "NSString+Categories.h"

@implementation NSString (MapsMeSize)

- (CGSize)sizeWithDrawSize:(CGSize)drawSize font:(UIFont *)font
{
  CGRect rect = [self boundingRectWithSize:drawSize options:NSStringDrawingUsesLineFragmentOrigin attributes:@{NSFontAttributeName : font} context:nil];
  return CGRectIntegral(rect).size;
}

@end

@implementation NSString (MapsMeRanges)

- (vector<NSRange>)rangesOfString:(NSString *)aString
{
  vector<NSRange> r;
  if (self.length == 0)
    return r;

  NSRange searchRange = {0, self.length};
  while (searchRange.location < self.length)
  {
    searchRange.length = self.length - searchRange.location;
    NSRange const foundRange = [self rangeOfString:aString options:NSCaseInsensitiveSearch range:searchRange];
    if (foundRange.location == NSNotFound)
      break;
    searchRange.location = foundRange.location + foundRange.length;
    r.push_back(foundRange);
  }
  return r;
}

@end
