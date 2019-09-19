#include <vector>

@interface NSString (MapsMeSize)

- (CGSize)sizeWithDrawSize:(CGSize)size font:(UIFont *)font;

@end

@interface NSString (MapsMeRanges)

- (std::vector<NSRange>)rangesOfString:(NSString *)aString;

@end
