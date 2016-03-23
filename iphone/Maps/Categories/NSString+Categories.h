#include "std/vector.hpp"

@interface NSString (MapsMeSize)

- (CGSize)sizeWithDrawSize:(CGSize)size font:(UIFont *)font;

@end

@interface NSString (MapsMeRanges)

- (vector<NSRange>)rangesOfString:(NSString *)aString;

@end