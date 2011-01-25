//
//  TestsAppDelegate.h
//  Tests
//
//  Created by Siarhei Rachytski on 1/25/11.
//  Copyright 2011 Credo-Dialogue. All rights reserved.
//

#import <UIKit/UIKit.h>

@class TestsViewController;

@interface TestsAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    TestsViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet TestsViewController *viewController;

@end

