//
//  AvidVideoPlaybackListener.h
//  AppVerificationLibrary
//
//  Created by Evgeniy Gubin on 22.06.16.
//  Copyright © 2016 Integral. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol MoPub_AvidVideoPlaybackListener <NSObject>

- (void)recordAdImpressionEvent;
- (void)recordAdStartedEvent;
- (void)recordAdLoadedEvent;
- (void)recordAdVideoStartEvent;
- (void)recordAdStoppedEvent;
- (void)recordAdCompleteEvent;
- (void)recordAdClickThruEvent;
- (void)recordAdVideoFirstQuartileEvent;
- (void)recordAdVideoMidpointEvent;
- (void)recordAdVideoThirdQuartileEvent;
- (void)recordAdPausedEvent;
- (void)recordAdPlayingEvent;
- (void)recordAdExpandedChangeEvent;
- (void)recordAdUserMinimizeEvent;
- (void)recordAdUserAcceptInvitationEvent;
- (void)recordAdUserCloseEvent;
- (void)recordAdSkippedEvent;
- (void)recordAdVolumeChangeEvent:(NSInteger)volume;
- (void)recordAdEnteredFullscreenEvent;
- (void)recordAdExitedFullscreenEvent;
- (void)recordAdDurationChangeEvent:(NSString *)adDuration  adRemainingTime:(NSString *)adRemainingTime;
- (void)recordAdErrorWithMessage:(NSString *)message;

@end
