//
//  MTRGInstreamAudioAdPlayer.h
//  MyTargetSDK
//
//  Created by Andrey Seredkin on 20.12.16.
//  Copyright © 2016 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol MTRGInstreamAudioAdPlayerDelegate <NSObject>

- (void)onAdAudioStart;

- (void)onAdAudioPause;

- (void)onAdAudioResume;

- (void)onAdAudioStop;

- (void)onAdAudioErrorWithReason:(NSString *)reason;

- (void)onAdAudioComplete;

@end

@protocol MTRGInstreamAudioAdPlayer <NSObject>

@property(nonatomic, readonly) NSTimeInterval adAudioDuration;
@property(nonatomic, readonly) NSTimeInterval adAudioTimeElapsed;
@property(nonatomic, weak) id <MTRGInstreamAudioAdPlayerDelegate> adPlayerDelegate;
@property(nonatomic) float volume;

- (void)playAdAudioWithUrl:(NSURL *)url;

- (void)pauseAdAudio;

- (void)resumeAdAudio;

- (void)stopAdAudio;

@end
