//
//  BenchmarksAppDelegate.h
//  Benchmarks
//
//  Created by Siarhei Rachytski on 4/11/11.
//  Copyright 2011 Credo-Dialogue. All rights reserved.
//

#import <UIKit/UIKit.h>

@class BenchmarksViewController;

@interface BenchmarksAppDelegate : NSObject <UIApplicationDelegate> {

}

@property (nonatomic, retain) IBOutlet UIWindow *window;

@property (nonatomic, retain) IBOutlet BenchmarksViewController *viewController;

@end
