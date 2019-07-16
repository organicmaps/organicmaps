//
//  PWRichMediaStyle.h
//  Pushwoosh SDK
//  (c) Pushwoosh 2018
//

#import <Foundation/Foundation.h>

FOUNDATION_EXPORT NSTimeInterval const PWRichMediaStyleDefaultAnimationDuration;

/**
 Interface for Rich Media Custom Animation.
 */
@protocol PWRichMediaStyleAnimationDelegate <NSObject>

/**
 This method can be used to animate Rich Media presenting view.
 */
- (void)runPresentingAnimationWithContentView:(UIView *)contentView parentView:(UIView *)parentView completion:(dispatch_block_t)completion;

/**
 This method can be used to animate Rich Media dismissing view.
 */
- (void)runDismissingAnimationWithContentView:(UIView *)contentView parentView:(UIView *)parentView completion:(dispatch_block_t)completion;

@end

/**
 Built-in Rich Media presenting animations.
 
 Example:
 
 style.animationDelegate = [PWRichMediaStyleSlideLeftAnimation new];
 
 */
@interface PWRichMediaStyleSlideLeftAnimation : NSObject <PWRichMediaStyleAnimationDelegate>
@end

@interface PWRichMediaStyleSlideRightAnimation : NSObject <PWRichMediaStyleAnimationDelegate>
@end

@interface PWRichMediaStyleSlideTopAnimation : NSObject <PWRichMediaStyleAnimationDelegate>
@end

@interface PWRichMediaStyleSlideBottomAnimation : NSObject <PWRichMediaStyleAnimationDelegate>
@end

@interface PWRichMediaStyleCrossFadeAnimation : NSObject <PWRichMediaStyleAnimationDelegate>
@end

/**
 Custom Rich Media loading view. It is shown while Rich Media is loading.
 */
@interface PWLoadingView : UIView

@property (nonatomic) IBOutlet UIActivityIndicatorView *activityIndicatorView;
@property (nonatomic) IBOutlet UIButton *cancelLoadingButton;

@end


typedef PWLoadingView *(^PWRichMediaLoadingViewBlock)(void);


/**
 'PWRichMediaStyle' class allows customizing the appearance of Rich Media pages.
 */
@interface PWRichMediaStyle : NSObject

/**
 Background color of Rich Media pages.
 */
@property (nonatomic) UIColor *backgroundColor;

/**
 Delegate to manage Rich Media presenting animation.
 */
@property (nonatomic) id<PWRichMediaStyleAnimationDelegate> animationDelegate;

/**
 Block to customize Rich Media loading view.
 
 Example:
 
 style.loadingViewBlock = ^PWLoadingView *{
    return [[[NSBundle mainBundle] loadNibNamed:@"LoadingView" owner:self options:nil] lastObject];
 };
 
 */
@property (nonatomic) PWRichMediaLoadingViewBlock loadingViewBlock;

/**
 Delay of the close button presenting in seconds.
 */
@property (nonatomic) NSTimeInterval closeButtonPresentingDelay;

/**
 Should status bar to be hidden or not while Rich Media page is presented. Default is 'YES'.
 */
@property (nonatomic) BOOL shouldHideStatusBar;

/**
 A Boolean value that determines whether HTML5 videos play inline or use the native full-screen controller.
 */
@property (nonatomic) NSNumber *allowsInlineMediaPlayback;

/**
 A Boolean value that determines whether HTML5 videos can play automatically or require the user to start playing them.
 */
@property (nonatomic) NSNumber *mediaPlaybackRequiresUserAction;

@end

