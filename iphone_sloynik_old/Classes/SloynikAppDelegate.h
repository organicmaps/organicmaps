//
//  SloynikAppDelegate.h
//  Sloynik
//
//  Created by Yury Melnichek on 07.09.09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

@interface SloynikAppDelegate : NSObject <UIApplicationDelegate>
{    
    UIWindow * window;
    UINavigationController * navigationController;
}

@property (nonatomic, retain) IBOutlet UIWindow * window;
@property (nonatomic, retain) IBOutlet UINavigationController * navigationController;

@end

