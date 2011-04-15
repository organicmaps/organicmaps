//
//  GuideViewController.h
//  Maps
//
//  Created by Yury Melnichek on 15.03.11.
//  Copyright 2011 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "../../Sloynik/Shared/SearchVC.h"

@interface GuideViewController : SearchVC
{
  UIActivityIndicatorView * activityIndicator;
  UILabel * loadingLabel;
}

@property (nonatomic, retain) UIActivityIndicatorView * activityIndicator;
@property (nonatomic, retain) UILabel * loadingLabel;

- (IBAction)OnMapClicked:(id)sender;
- (void)OnSloynikEngineInitialized;

@end
