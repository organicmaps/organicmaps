//
//  QuartzBenchmarkAppDelegate.h
//  QuartzBenchmark
//
//  Created by Siarhei Rachytski on 4/12/11.
//  Copyright 2011 Credo-Dialogue. All rights reserved.
//

#import <UIKit/UIKit.h>

@class QuartzBenchmarkViewController;

@interface QuartzBenchmarkAppDelegate : NSObject <UIApplicationDelegate> {

}

@property (nonatomic, retain) IBOutlet UIWindow *window;

@property (nonatomic, retain) IBOutlet QuartzBenchmarkViewController *viewController;

@end
