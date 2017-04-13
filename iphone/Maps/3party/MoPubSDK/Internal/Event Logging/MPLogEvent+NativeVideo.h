//
//  MPLogEvent+NativeVideo.h
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "MPLogEvent.h"

typedef NS_ENUM(NSInteger, MPNativeVideoEventType) {
    MPNativeVideoEventTypeDownloadStart,
    MPNativeVideoEventTypeVideoReady,
    MPNativeVideoEventTypeBuffering,
    MPNativeVideoEventTypeDownloadFinished,
    MPNativeVideoEventTypeErrorFailedToPlay,
    MPNativeVideoEventTypeErrorDuringPlayback,
};

@interface MPLogEvent (NativeVideo)

- (instancetype)initWithLogEventProperties:(MPAdConfigurationLogEventProperties *)logEventProperties nativeVideoEventType:(MPNativeVideoEventType)nativeVideoEventType;

@end
