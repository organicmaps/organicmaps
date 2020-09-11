//
//  MPProgressOverlayView.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

@protocol MPProgressOverlayViewDelegate;

/**
 Progress overlay meant for display over the key window of the application.
 */
@interface MPProgressOverlayView : UIView
/**
 Optional delegate to listen for progress overlay events.
 */
@property (nonatomic, weak) id<MPProgressOverlayViewDelegate> delegate;

/**
 Initializes the progress overlay with an optional delegate.
 @param delegate Optional delegate to listen for progress overlay events.
 @return A progress overlay instance.
 */
- (instancetype)initWithDelegate:(id<MPProgressOverlayViewDelegate>)delegate;

/**
 Shows the progress overlay over the key window.
 */
- (void)show;

/**
 Removes the progress overlay from the key window.
 */
- (void)hide;
@end

#pragma mark - MPProgressOverlayViewDelegate

@protocol MPProgressOverlayViewDelegate <NSObject>
@optional
/**
 Cancel button pressed.
 */
- (void)overlayCancelButtonPressed;

/**
 Progress overlay completed animating on screen.
 */
- (void)overlayDidAppear;
@end
