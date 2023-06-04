#import <CoreApi/MWMStorage.h>

NS_ASSUME_NONNULL_BEGIN

@interface MWMStorage (UI)

- (void)downloadNode:(NSString *)countryId;
- (void)downloadNode:(NSString *)countryId onSuccess:(nullable MWMVoidBlock)success;
- (void)updateNode:(NSString *)countryId;
- (void)updateNode:(NSString *)countryId onCancel:(nullable MWMVoidBlock)cancel;
- (void)deleteNode:(NSString *)countryId;
- (void)downloadNodes:(NSArray<NSString *> *)countryIds onSuccess:(nullable MWMVoidBlock)success;

@end

NS_ASSUME_NONNULL_END
