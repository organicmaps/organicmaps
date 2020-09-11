//
//  MPConsentDialogViewController.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class MPConsentDialogViewController;

@protocol MPConsentDialogViewControllerDelegate<NSObject>

@optional

/**
 Informs the delegate of the received consent response.
 */
- (void)consentDialogViewControllerDidReceiveConsentResponse:(BOOL)response
                                 consentDialogViewController:(MPConsentDialogViewController *)consentDialogViewController;

/**
 Informs the delegate that the given consentDialogViewController will disappear.
 */
- (void)consentDialogViewControllerWillDisappear:(MPConsentDialogViewController *)consentDialogViewController;

/**
 Informs the delegate that the given consentDialogViewController did complete dismissing.
 */
- (void)consentDialogViewControllerDidDismiss:(MPConsentDialogViewController *)consentDialogViewController;

@end

@interface MPConsentDialogViewController : UIViewController

/**
 Initializes a consent dialog view controller with an HTML string to load. It is expected that this initializer is used
 when initializing.

 @param dialogHTML The markup string for the consent dialog.
 */
- (instancetype)initWithDialogHTML:(NSString *)dialogHTML NS_DESIGNATED_INITIALIZER;

/**
 Delegate object to inform an outside object of events.
 */
@property (nonatomic, weak) id<MPConsentDialogViewControllerDelegate> delegate;

/**
 This method starts the loading of the consent page. When complete, `completion` will be called with a `BOOL`
 indicating success, and an `NSError` object with information about the error in the case of failure.

 This method can be called and is expected to be called before the view controller is presented. This way,
 the consent dialog is already loaded and in-view by the time the user sees it.
 */
- (void)loadConsentPageWithCompletion:(void (^_Nullable)(BOOL success, NSError *error))completion;

/**
 These initializers are not available
 */
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder *)aDecoder NS_UNAVAILABLE;
- (instancetype)initWithNibName:(NSString * _Nullable)nibNameOrNil bundle:(NSBundle * _Nullable)nibBundleOrNil NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
