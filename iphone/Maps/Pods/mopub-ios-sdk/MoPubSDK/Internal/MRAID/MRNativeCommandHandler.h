//
//  MRNativeCommandHandler.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MRConstants.h"

@class MRCommand;
@protocol MRNativeCommandHandlerDelegate;

/**
 * The `MRNativeCommandHandler` class is an object that encapsulates functionality that processes,
 * and where possible, executes MRAID commands.
 */
@interface MRNativeCommandHandler : NSObject

- (instancetype)initWithDelegate:(id<MRNativeCommandHandlerDelegate>)delegate;
- (void)handleNativeCommand:(NSString *)command withProperties:(NSDictionary *)properties;

@end

/**
 * The delegate of an `MRNativeCommandHandler` object that implements `MRNativeCommandHandlerDelegate`
 * must provide information and a view controller that allow the `MRNativeCommandHandler` to execute
 * MRAID commands. The `MRNativeCommandHandlerDelegate` is also notified of certain events and
 * expected to respond appropriately to them.
 */
@protocol MRNativeCommandHandlerDelegate <NSObject>

- (void)handleMRAIDUseCustomClose:(BOOL)useCustomClose;
- (void)handleMRAIDSetOrientationPropertiesWithForceOrientationMask:(UIInterfaceOrientationMask)forceOrientationMask;
- (void)handleMRAIDExpandWithParameters:(NSDictionary *)params;
- (void)handleMRAIDResizeWithParameters:(NSDictionary *)params;
- (void)handleMRAIDClose;
- (void)handleMRAIDOpenCallForURL:(NSURL *)URL;
- (void)nativeCommandWillPresentModalView;
- (void)nativeCommandDidDismissModalView;
- (void)nativeCommandCompleted:(NSString *)command;
- (void)nativeCommandFailed:(NSString *)command withMessage:(NSString *)message;
- (UIViewController *)viewControllerForPresentingModalView;
- (MRAdViewPlacementType)adViewPlacementType;
- (BOOL)userInteractedWithWebView;
- (BOOL)handlingWebviewRequests;

@end
