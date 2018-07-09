//
//  MTRGInstreamAdPlayer.h
//  myTarget
//
//  Created by Anton Bulankin on 21.09.16.
//  Copyright © 2016 Mail.ru. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol MTRGInstreamAdPlayerDelegate <NSObject>

- (void)onAdVideoStart;

- (void)onAdVideoPause;

- (void)onAdVideoResume;

- (void)onAdVideoStop;

- (void)onAdVideoErrorWithReason:(NSString *)reason;

- (void)onAdVideoComplete;

@end

@protocol MTRGInstreamAdPlayer <NSObject>

@property(nonatomic, readonly) NSTimeInterval adVideoDuration;
@property(nonatomic, readonly) NSTimeInterval adVideoTimeElapsed;
@property(nonatomic, weak) id <MTRGInstreamAdPlayerDelegate> adPlayerDelegate;
@property(nonatomic, readonly) UIView *adPlayerView;
@property(nonatomic) float volume;

- (void)playAdVideoWithUrl:(NSURL *)url;

- (void)pauseAdVideo;

- (void)resumeAdVideo;

- (void)stopAdVideo;

@end
