#import <UIKit/UIKit.h>

//! Protocol for customizing Aarki offers and providing offer request callbacks
/*!
 
 The \a didExit callback is called whenever the user leaves the view controller.
 This may happen when the user successfully completes, quits or abandons the offer.
 */
@class AarkiOffer;

@protocol AarkiDelegate<NSObject>

@optional
- (void)didExit;

@end

@protocol AarkiOfferDisplay<NSObject>

@required

//! Show the offer (as a full screen modal overlay)
/*!
 \param viewController For example the controller of the view which contains the ad which user clicks to present this offer
 
 This method is essentially equivalent to:
 \code
 [viewController presentModalViewController:self animated:animated];
 \endcode
 
 The offer will be dismissed with animation if it was presented with animation.
 \sa dismiss
 */
- (void)presentWithParent:(UIViewController *)viewController animated:(BOOL)animated;

//! Hide the offer. You will not need to call this method explicitly.
- (void)dismiss;

//! Internal
- (void)setViewOptions:(NSDictionary *)dict;


@property (assign) id<AarkiDelegate> loaderDelegate;

@end


typedef enum {
    AarkiStatusOK,
    AarkiStatusNotAvailable,
    AarkiStatusAppNotRegistered
} AarkiStatus;

typedef void(^AarkiLoaderCompletionBlock)(AarkiStatus);


//! Class to request Aarki offers
@interface Aarki : NSObject <AarkiDelegate>
{
@public
    id<AarkiOfferDisplay> offerViewController;
    id<AarkiDelegate> delegate;
}

@property (nonatomic, retain) id<AarkiOfferDisplay> offerViewController;
@property (nonatomic, assign) id<AarkiDelegate> delegate;

@property (nonatomic, copy) AarkiLoaderCompletionBlock  completionBlock_;

//! Initialize
- (id)init;


// Engage-to-earn video and rich media ads
- (void)fetchFullScreenAd:(NSString *)placementId
                  options:(NSDictionary *)options
               completion:(AarkiLoaderCompletionBlock)completion;

- (void)showFullScreenAd:(NSString *)placementId
              withParent:(UIViewController *)parentViewController
                 options:(NSDictionary *)options;

- (void)showFullScreenAd:(NSString *)placementId
              withParent:(UIViewController *)parentViewController
                 options:(NSDictionary *)options
              completion:(AarkiLoaderCompletionBlock)completion;

// Interstitials
- (void)fetchInterstitialAd:(NSString *)placementId
                    options:(NSDictionary *)options
                 completion:(AarkiLoaderCompletionBlock)completion;

- (void)showInterstitialAd:(NSString *)placementId
                withParent:(UIViewController *)parentViewController
                   options:(NSDictionary *)options
                completion:(AarkiLoaderCompletionBlock)completion;

// Offer Deck
- (void)showAds:(NSString *)placementId
     withParent:(UIViewController *)parentViewController
        options:(NSDictionary *)options;


- (id<AarkiOfferDisplay>)nativeViewControllerForPlacement:(NSString *)placementId options:(NSDictionary *)options;


@end

