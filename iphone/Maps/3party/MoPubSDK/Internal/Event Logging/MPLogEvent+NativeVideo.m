//
//  MPLogEvent+NativeVideo.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPLogEvent+NativeVideo.h"

static NSDictionary *nativeVideoLogEventTypeToString = nil;

NSString *const MPNativeVideoEventTypeDownloadStartString = @"download_start";
NSString *const MPNativeVideoEventTypeVideoReadyString = @"download_video_ready";
NSString *const MPNativeVideoEventTypeBufferingString = @"download_buffering";
NSString *const MPNativeVideoEventTypeDownloadFinishedString = @"download_finished";
NSString *const MPNativeVideoEventTypeErrorFailedToPlayString = @"error_failed_to_play";
NSString *const MPNativeVideoEventTypeErrorDuringPlaybackString = @"error_during_playback";

@implementation MPLogEvent (NativeVideo)

- (instancetype)initWithLogEventProperties:(MPAdConfigurationLogEventProperties *)logEventProperties nativeVideoEventType:(MPNativeVideoEventType)nativeVideoEventType
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        nativeVideoLogEventTypeToString = @{@(MPNativeVideoEventTypeDownloadStart): MPNativeVideoEventTypeDownloadStartString,
                                            @(MPNativeVideoEventTypeVideoReady): MPNativeVideoEventTypeVideoReadyString,
                                            @(MPNativeVideoEventTypeBuffering): MPNativeVideoEventTypeBufferingString,
                                            @(MPNativeVideoEventTypeDownloadFinished): MPNativeVideoEventTypeDownloadFinishedString,
                                            @(MPNativeVideoEventTypeErrorDuringPlayback): MPNativeVideoEventTypeErrorDuringPlaybackString,
                                            @(MPNativeVideoEventTypeErrorFailedToPlay): MPNativeVideoEventTypeErrorFailedToPlayString};
    });

    if (self = [[MPLogEvent alloc] initWithEventCategory:MPLogEventCategoryNativeVideo eventName:[nativeVideoLogEventTypeToString objectForKey:@(nativeVideoEventType)]]) {
        [self setLogEventProperties:logEventProperties];
    }
    return self;
}

@end
