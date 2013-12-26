//
//  MPAdWebView.m
//  MoPub
//
//  Copyright (c) 2012 MoPub, Inc. All rights reserved.
//

#import "MPAdWebView.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPAdWebView

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        self.backgroundColor = [UIColor clearColor];
        self.opaque = NO;

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= MP_IOS_4_0
        if ([self respondsToSelector:@selector(allowsInlineMediaPlayback)]) {
            [self setAllowsInlineMediaPlayback:YES];
            [self setMediaPlaybackRequiresUserAction:NO];
        }
#endif
    }
    return self;
}

@end
