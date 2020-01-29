//
//  MPNativeAdRendererImageHandler.h
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@protocol MPNativeAdRendererImageHandlerDelegate <NSObject>

- (BOOL)nativeAdViewInViewHierarchy;

@end

@interface MPNativeAdRendererImageHandler : NSObject


@property (nonatomic, weak) id<MPNativeAdRendererImageHandlerDelegate> delegate;

- (void)loadImageForURL:(NSURL *)imageURL intoImageView:(UIImageView *)imageView;

@end
