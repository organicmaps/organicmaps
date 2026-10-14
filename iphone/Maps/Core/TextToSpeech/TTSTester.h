NS_ASSUME_NONNULL_BEGIN

@interface TTSTester : NSObject

- (nullable NSString *)nextTestString:(NSString *)language;
- (nullable NSArray<NSString *> *)getTestStrings:(NSString *)language;

@end

NS_ASSUME_NONNULL_END
