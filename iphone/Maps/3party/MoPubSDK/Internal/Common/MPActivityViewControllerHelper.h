//
//  MPActivityViewControllerHelper.h
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>

@protocol MPActivityViewControllerHelperDelegate;

/**
 * The MPActivityViewControllerHelper provides a wrapper around a UIActvityViewController
 * and provides hooks via the MPActivityViewControllerHelperDelegate to handle the
 * lifecycle of the underlying UIActivityViewController.
 */

@interface MPActivityViewControllerHelper : NSObject

/**
 * The delegate (`MPActivityViewControllerHelperDelegate`) of the
 * MPActivityViewControllerHelper.
 */

@property (nonatomic, weak) id<MPActivityViewControllerHelperDelegate> delegate;

/**
 * Initializes the MPActivityViewControllerHelper and stores a weak reference
 * to the supplied delegate.
 *
 * @param delegate
 */
- (instancetype)initWithDelegate:(id<MPActivityViewControllerHelperDelegate>)delegate;

/**
 * Instantiates and displays the underlying UIActivityViewController with the
 * the specified `subject` and `body`.
 *
 * @param subject The subject to be displayed in the UIActivityViewController.
 * @param body The body to be displayed in the UIActivityViewController.
 *
 * @return a BOOL indicating whether or not the UIActivityViewController was successfully shown.
 */
- (BOOL)presentActivityViewControllerWithSubject:(NSString *)subject body:(NSString *)body;

@end


/**
 * The delegate of a `MPActivityViewController` must adopt the `MPActivityViewController`
 * protocol. It must implement `viewControllerForPresentingActivityViewController` to
 * provide a root view controller from which to display content.
 *
 * Optional methods of this protocol allow the delegate to be notified before
 * presenting and after dismissal.
 */
@protocol MPActivityViewControllerHelperDelegate <NSObject>

@required

/**
 * Asks the delegate for a view controller to use for presenting content.
 *
 * @return A view controller that should be used for presenting content.
 */
- (UIViewController *)viewControllerForPresentingActivityViewController;

@optional

/**
 * Sent before the UIActivityViewController is presented.
 */
- (void)activityViewControllerWillPresent;

/**
 * Sent after the UIActivityViewController has been dismissed.
 */
- (void)activityViewControllerDidDismiss;

@end
