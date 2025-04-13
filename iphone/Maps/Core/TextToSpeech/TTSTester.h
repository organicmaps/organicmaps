NS_ASSUME_NONNULL_BEGIN

@interface TTSTester : NSObject

- (void)playRandomTestString;
- (NSArray<NSString *> *)getTestStrings:(NSString *)language;

@end

NS_ASSUME_NONNULL_END
