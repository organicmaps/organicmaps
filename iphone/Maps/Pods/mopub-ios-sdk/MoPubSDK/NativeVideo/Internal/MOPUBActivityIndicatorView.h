//
//  MOPUBActivityIndicatorView.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

@interface MOPUBActivityIndicatorView : UIView

- (void)startAnimating;
- (void)stopAnimating;
- (BOOL)isAnimating;

@end
