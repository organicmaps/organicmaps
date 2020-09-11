//
//  MPNativeView.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

@protocol MPNativeViewDelegate;

@interface MPNativeView : UIView

@property (nonatomic, weak) id<MPNativeViewDelegate> delegate;

@end

@protocol MPNativeViewDelegate <NSObject>

@required

- (void)nativeViewWillMoveToSuperview:(UIView *)superview;

@end
