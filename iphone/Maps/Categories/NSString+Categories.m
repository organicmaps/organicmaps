#import "NSString+Categories.h"

@implementation NSString (MapsMeSize)

- (CGSize)sizeWithDrawSize:(CGSize)drawSize font:(UIFont *)font {
  CGRect rect = [self boundingRectWithSize:drawSize
                                   options:NSStringDrawingUsesLineFragmentOrigin
                                attributes:@{NSFontAttributeName : font}
                                   context:nil];
  return CGRectIntegral(rect).size;
}

@end

@implementation NSString (MapsMeRanges)

- (NSArray<NSValue *> *)rangesOfString:(NSString *)aString {
  NSMutableArray *result = [NSMutableArray array];
  if (self.length == 0) {
    return [result copy];
  }

  NSRange searchRange = NSMakeRange(0, self.length);
  while (searchRange.location < self.length) {
    searchRange.length = self.length - searchRange.location;
    NSRange foundRange = [self rangeOfString:aString options:NSCaseInsensitiveSearch range:searchRange];
    if (foundRange.location == NSNotFound) {
      break;
    }
    searchRange.location = foundRange.location + foundRange.length;
    [result addObject:[NSValue valueWithRange:foundRange]];
  }
  return result;
}

@end
