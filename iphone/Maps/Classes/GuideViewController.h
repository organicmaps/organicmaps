//
//  GuideViewController.h
//  Maps
//
//  Created by Yury Melnichek on 15.03.11.
//  Copyright 2011 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>

@class SearchVC;

@interface GuideViewController : UIViewController
{
  SearchVC * searchVC;
}

@property (nonatomic, retain) SearchVC * searchVC;

- (IBAction)OnMapClicked:(id)sender;

@end
