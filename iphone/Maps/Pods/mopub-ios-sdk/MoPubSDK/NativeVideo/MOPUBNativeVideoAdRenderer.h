//
//  MOPUBNativeVideoAdRenderer.h
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MPNativeAdRenderer.h"

@class MPNativeAdRendererConfiguration;
@class MPStaticNativeAdRendererSettings;

@interface MOPUBNativeVideoAdRenderer : NSObject

@property (nonatomic, readonly) MPNativeViewSizeHandler viewSizeHandler;

+ (MPNativeAdRendererConfiguration *)rendererConfigurationWithRendererSettings:(id<MPNativeAdRendererSettings>)rendererSettings;

@end
