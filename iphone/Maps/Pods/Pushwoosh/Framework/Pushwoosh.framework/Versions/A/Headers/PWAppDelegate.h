//
//  PWAppDelegate.h
//  Pushwoosh SDK
//  (c) Pushwoosh 2018
//

#if TARGET_OS_IPHONE

#import <UIKit/UIKit.h>

/*
 Base AppDelegate class for easier integration.
 */
@interface PWAppDelegate : UIResponder <UIApplicationDelegate>

@property (nonatomic, strong) UIWindow *window;

@end

#endif
