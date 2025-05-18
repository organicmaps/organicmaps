#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface PlacePagePhone : NSObject

/// User-visible string of the phone number.
@property(nonatomic, readonly) NSString *phone;
/// Optional `tel:` URL, which can be used to ask iOS to call the phone number.
@property(nonatomic, readonly, nullable) NSURL *url;

+ (instancetype)placePagePhoneWithPhone:(NSString *)phone andURL:(nullable NSURL *)url;

@end

NS_ASSUME_NONNULL_END
