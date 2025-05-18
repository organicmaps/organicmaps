#import "PlacePagePhone.h"

@implementation PlacePagePhone

- (instancetype)initWithPhone:(NSString *)phone andURL:(nullable NSURL *)url {
    self = [super init];
    if (self) {
        _phone = phone;
        _url = url;
    }
    return self;
}

+ (instancetype)placePagePhoneWithPhone:(NSString *)phone andURL:(nullable NSURL *)url {
    return [[self alloc] initWithPhone:phone andURL:url];
}

@end
