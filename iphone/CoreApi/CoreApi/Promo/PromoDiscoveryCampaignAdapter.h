#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface PromoDiscoveryCampaignAdapter : NSObject

@property(nonatomic, retain, nullable) NSURL *url;
@property(nonatomic) NSInteger type;

- (instancetype)init;
- (BOOL)canShowTipButton;

@end

NS_ASSUME_NONNULL_END
