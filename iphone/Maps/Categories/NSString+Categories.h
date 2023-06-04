@interface NSString (MapsMeSize)

- (CGSize)sizeWithDrawSize:(CGSize)size font:(UIFont *)font;

@end

@interface NSString (MapsMeRanges)

- (NSArray<NSValue *> *)rangesOfString:(NSString *)aString;

@end
