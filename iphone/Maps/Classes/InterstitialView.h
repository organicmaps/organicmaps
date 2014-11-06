
#import <UIKit/UIKit.h>

typedef NS_ENUM(NSUInteger, InterstitialViewResult) {
  InterstitialViewResultDismissed,
  InterstitialViewResultClicked
};

@class InterstitialView;
@protocol InterstitialViewDelegate <NSObject>

- (void)interstitialView:(InterstitialView *)interstitial didCloseWithResult:(InterstitialViewResult)result;
- (void)interstitialViewWillOpen:(InterstitialView *)interstitial;

@end

@interface InterstitialView : UIView

@property (nonatomic, weak, readonly) id <InterstitialViewDelegate> delegate;
@property (nonatomic, readonly) NSString * inAppMessageName;
@property (nonatomic, readonly) NSString * imageType;

// Portrait, Landscape
- (instancetype)initWithImages:(NSArray *)images inAppMessageName:(NSString *)messageName imageType:(NSString *)imageType delegate:(id <InterstitialViewDelegate>)delegate;
- (void)show;

@end