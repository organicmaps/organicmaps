//
//  MPClosableView.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

extern const CGSize kCloseRegionSize;

enum {
    MPClosableViewCloseButtonLocationTopRight,
    MPClosableViewCloseButtonLocationTopLeft,
    MPClosableViewCloseButtonLocationTopCenter,
    MPClosableViewCloseButtonLocationBottomRight,
    MPClosableViewCloseButtonLocationBottomLeft,
    MPClosableViewCloseButtonLocationBottomCenter,
    MPClosableViewCloseButtonLocationCenter
};
typedef NSInteger MPClosableViewCloseButtonLocation;

enum {
    MPClosableViewCloseButtonTypeNone,
    MPClosableViewCloseButtonTypeTappableWithoutImage,
    MPClosableViewCloseButtonTypeTappableWithImage,
};
typedef NSInteger MPClosableViewCloseButtonType;

CGRect MPClosableViewCustomCloseButtonFrame(CGSize size, MPClosableViewCloseButtonLocation closeButtonLocation);

@protocol MPClosableViewDelegate;

/**
 * `MPClosableView` is composed of a content view and a close button. The close button can
 * be positioned at any corner, the center of top and bottom edges, and the center of the view.
 * The close button can either be a button that is tappable with image, tappable without an image,
 * and completely disabled altogether. Finally, `MPClosableView` keeps track as to whether or not
 * it has been tapped.
 */
@interface MPClosableView : UIView

@property (nonatomic, weak) id<MPClosableViewDelegate> delegate;
@property (nonatomic, assign) MPClosableViewCloseButtonType closeButtonType;
@property (nonatomic, assign) MPClosableViewCloseButtonLocation closeButtonLocation;
@property (nonatomic, readonly) BOOL wasTapped;
@property (nonatomic, strong, readonly) UIButton *closeButton;

- (instancetype)initWithFrame:(CGRect)frame
                  contentView:(UIView *)contentView
                     delegate:(id<MPClosableViewDelegate>)delegate;

@end

/**
 * `MPClosableViewDelegate` allows an object that implements `MPClosableViewDelegate` to
 * be notified when the close button of the `MPClosableView` has been tapped.
 */
@protocol MPClosableViewDelegate <NSObject>

- (void)closeButtonPressed:(MPClosableView *)closableView;

@optional

- (void)closableView:(MPClosableView *)closableView didMoveToWindow:(UIWindow *)window;

@end
