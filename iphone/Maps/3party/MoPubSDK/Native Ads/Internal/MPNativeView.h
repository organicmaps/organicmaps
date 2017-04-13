//
//  MPNativeView.h
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
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
