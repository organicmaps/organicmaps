//
//  MPImageLoader.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class MPImageLoader;

@protocol MPImageLoaderDelegate <NSObject>

- (BOOL)nativeAdViewInViewHierarchy;

@optional

- (void)imageLoader:(MPImageLoader *)imageLoaded didLoadImageIntoImageView:(UIImageView *)imageView;

@end

@interface MPImageLoader : NSObject

@property (nonatomic, weak) id<MPImageLoaderDelegate> delegate;

- (void)loadImageForURL:(NSURL *)imageURL intoImageView:(UIImageView *)imageView;

@end
