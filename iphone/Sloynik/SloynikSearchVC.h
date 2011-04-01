//
//  SloynikSearchVC.h
//  Sloynik
//
//  Created by Yury Melnichek on 01.04.11.
//  Copyright 2011 -. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "SearchVC.h"

@interface SloynikSearchVC : SearchVC
{
  UIBarButtonItem * menuButton;
}

@property (nonatomic, retain) UIBarButtonItem * menuButton;

- (void)menuButtonPressed;

@end
