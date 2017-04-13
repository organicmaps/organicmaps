//
//  MPBReplayView.h
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MOPUBPlayerView.h"

@class MOPUBReplayView;

typedef void (^MPBReplayActionBlock)(MOPUBReplayView *replayView);

@interface MOPUBReplayView : UIView

@property (nonatomic, copy) MPBReplayActionBlock actionBlock;

- (instancetype)initWithFrame:(CGRect)frame displayMode:(MOPUBPlayerDisplayMode)displayMode;

@end
