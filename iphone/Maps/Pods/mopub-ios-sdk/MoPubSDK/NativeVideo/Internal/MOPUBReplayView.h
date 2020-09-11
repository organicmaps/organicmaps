//
//  MOPUBReplayView.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MOPUBPlayerView.h"

@class MOPUBReplayView;

typedef void (^MPBReplayActionBlock)(MOPUBReplayView *replayView);

@interface MOPUBReplayView : UIView

@property (nonatomic, copy) MPBReplayActionBlock actionBlock;

- (instancetype)initWithFrame:(CGRect)frame displayMode:(MOPUBPlayerDisplayMode)displayMode;

@end
