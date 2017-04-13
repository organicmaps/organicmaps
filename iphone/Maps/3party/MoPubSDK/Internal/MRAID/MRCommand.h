//
//  MRCommand.h
//  MoPub
//
//  Copyright (c) 2011 MoPub, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class MRCommand;

@protocol MRCommandDelegate <NSObject>

- (void)mrCommand:(MRCommand *)command createCalendarEventWithParams:(NSDictionary *)params __deprecated;
- (void)mrCommand:(MRCommand *)command playVideoWithURL:(NSURL *)url;
- (void)mrCommand:(MRCommand *)command storePictureWithURL:(NSURL *)url __deprecated;
- (void)mrCommand:(MRCommand *)command shouldUseCustomClose:(BOOL)useCustomClose;
- (void)mrCommand:(MRCommand *)command setOrientationPropertiesWithForceOrientation:(UIInterfaceOrientationMask)forceOrientation;
- (void)mrCommand:(MRCommand *)command openURL:(NSURL *)url;
- (void)mrCommand:(MRCommand *)command expandWithParams:(NSDictionary *)params;
- (void)mrCommand:(MRCommand *)command resizeWithParams:(NSDictionary *)params;
- (void)mrCommandClose:(MRCommand *)command;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRCommand : NSObject

@property (nonatomic, weak) id<MRCommandDelegate> delegate;

+ (id)commandForString:(NSString *)string;

// returns YES by default for user safety
- (BOOL)requiresUserInteractionForPlacementType:(NSUInteger)placementType;
// This allows commands to run even if the delegate is not handling webview requests. Returns NO by default to avoid race conditions. This is
// primarily used to stop commands that can cause bad side effects while the mraid ad is presenting, dismissing, resizing, expanding and pretty much
// just animating at all. If you decide to return YES for this method, you must make sure that the command can operate safely at any point in time
// during an MRAID ad's lifetime from starting presentation to complete dismissal.
- (BOOL)executableWhileBlockingRequests;
- (BOOL)executeWithParams:(NSDictionary *)params;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRCloseCommand : MRCommand

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRExpandCommand : MRCommand

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRResizeCommand : MRCommand

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRUseCustomCloseCommand : MRCommand

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRSetOrientationPropertiesCommand : MRCommand

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MROpenCommand : MRCommand

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRCreateCalendarEventCommand : MRCommand

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRPlayVideoCommand : MRCommand

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRStorePictureCommand : MRCommand

@end
