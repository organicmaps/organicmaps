//
//  MRDimmingView.m
//  MoPub
//
//  Created by Andrew He on 12/19/11.
//  Copyright (c) 2011 MoPub, Inc. All rights reserved.
//

#import "MRDimmingView.h"

@implementation MRDimmingView

@synthesize dimmed = _dimmed;
@synthesize dimmingOpacity = _dimmingOpacity;

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        self.alpha = 0.0;
    }
    return self;
}

- (void)setDimmed:(BOOL)dimmed {
    _dimmed = dimmed;
    self.alpha = (_dimmed) ? _dimmingOpacity : 0.0;
}

- (void)setDimmingOpacity:(CGFloat)dimmingOpacity
{
    _dimmingOpacity = dimmingOpacity;
    [self setDimmed:self.dimmed];
}

@end
