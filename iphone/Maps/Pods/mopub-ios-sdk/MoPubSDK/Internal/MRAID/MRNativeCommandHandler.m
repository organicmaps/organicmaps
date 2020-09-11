//
//  MRNativeCommandHandler.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MRNativeCommandHandler.h"
#import "MRCommand.h"
#import "MPGlobal.h"
#import "MPLogging.h"
#import "MRVideoPlayerManager.h"

@interface MRNativeCommandHandler () <MRCommandDelegate, MRVideoPlayerManagerDelegate>

@property (nonatomic, weak) id <MRNativeCommandHandlerDelegate>delegate;
@property (nonatomic, strong) MRVideoPlayerManager *videoPlayerManager;

@end

@implementation MRNativeCommandHandler

- (instancetype)initWithDelegate:(id<MRNativeCommandHandlerDelegate>)delegate
{
    self = [super init];
    if (self) {
        _delegate = delegate;

        _videoPlayerManager = [[MRVideoPlayerManager alloc] initWithDelegate:self];
    }

    return self;
}

- (void)handleNativeCommand:(NSString *)command withProperties:(NSDictionary *)properties
{
    BOOL success = YES;

    MRCommand *cmd = [MRCommand commandForString:command];
    if (cmd == nil) {
        success = NO;
    } else if ([self shouldExecuteMRCommand:cmd]) {
        cmd.delegate = self;
        success = [cmd executeWithParams:properties];
    }

    [self.delegate nativeCommandCompleted:command];

    if (!success) {
        MPLogDebug(@"Unknown command: %@", command);
        [self.delegate nativeCommandFailed:command withMessage:@"Specified command is not implemented."];
    }
}

- (BOOL)shouldExecuteMRCommand:(MRCommand *)cmd
{
    // The command may not be whitelisted to run while the delegate is not handling webview requests.
    if (![self.delegate handlingWebviewRequests] && ![cmd executableWhileBlockingRequests]) {
        return NO;
    }

    // some MRAID commands may not require user interaction
    return ![cmd requiresUserInteractionForPlacementType:[self.delegate adViewPlacementType]] || [self.delegate userInteractedWithWebView];
}

#pragma mark - MRCommandDelegate

- (void)mrCommand:(MRCommand *)command playVideoWithURL:(NSURL *)url
{
    [self.videoPlayerManager playVideo:url];
}

- (void)mrCommand:(MRCommand *)command shouldUseCustomClose:(BOOL)useCustomClose
{
    [self.delegate handleMRAIDUseCustomClose:useCustomClose];
}

- (void)mrCommand:(MRCommand *)command setOrientationPropertiesWithForceOrientation:(UIInterfaceOrientationMask)forceOrientation
{
    [self.delegate handleMRAIDSetOrientationPropertiesWithForceOrientationMask:forceOrientation];
}

- (void)mrCommand:(MRCommand *)command openURL:(NSURL *)url
{
    [self.delegate handleMRAIDOpenCallForURL:url];
}

- (void)mrCommand:(MRCommand *)command expandWithParams:(NSDictionary *)params
{
    [self.delegate handleMRAIDExpandWithParameters:params];
}

- (void)mrCommand:(MRCommand *)command resizeWithParams:(NSDictionary *)params
{
    [self.delegate handleMRAIDResizeWithParameters:params];
}

- (void)mrCommandClose:(MRCommand *)command
{
    [self.delegate handleMRAIDClose];
}

#pragma mark - <MRVideoPlayerManagerDelegate>

- (void)videoPlayerManager:(MRVideoPlayerManager *)manager didFailToPlayVideoWithErrorMessage:(NSString *)message
{
    [self.delegate nativeCommandFailed:@"playVideo" withMessage:message];
}

- (void)videoPlayerManagerWillPresentVideo:(MRVideoPlayerManager *)manager
{
    [self.delegate nativeCommandWillPresentModalView];
}

- (void)videoPlayerManagerDidDismissVideo:(MRVideoPlayerManager *)manager
{
    [self.delegate nativeCommandDidDismissModalView];
}

- (UIViewController *)viewControllerForPresentingVideoPlayer
{
    return [self.delegate viewControllerForPresentingModalView];
}

@end
