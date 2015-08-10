//
//  MWMPedestrianShareAlert.m
//  Maps
//
//  Created by Ilya Grechuhin on 05.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Common.h"
#import "Macros.h"
#import "MWMActivityViewController.h"
#import "MWMAlertViewController.h"
#import "MWMPedestrianShareAlert.h"
#import "Statistics.h"
#import "UIButton+RuntimeAttributes.h"
#import "UIColor+MapsMeColor.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

static NSString * const kShareEventName = @"pedestrianShareEvent";

@interface MWMPedestrianShareAlert ()

@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet UIView * videoView;
@property (weak, nonatomic) IBOutlet UIImageView * image;
@property (weak, nonatomic) IBOutlet UILabel * message;
@property (weak, nonatomic) IBOutlet UIButton * shareButton;

@property (nonatomic) BOOL isFirstLaunch;

@end

@implementation MWMPedestrianShareAlert

+ (MWMPedestrianShareAlert *)alert:(BOOL)isFirstLaunch
{
  [Statistics.instance logEvent:[NSString stringWithFormat:@"%@ - %@", kShareEventName, @"open"]];
  MWMPedestrianShareAlert * view = [NSBundle.mainBundle loadNibNamed:NSStringFromClass(self.class) owner:self
                                                             options:nil].firstObject;
  view.isFirstLaunch = isFirstLaunch;
  if (isFirstLaunch)
  {
    view.title.text = L(@"dialog_title_share_walking_routes_first_launch");
    view.message.text = L(@"dialog_text_share_walking_routes_first_launch");
    view.image.image = [UIImage imageNamed:@"img_pedestrian"];
  }
  else
  {
    view.title.text = L(@"dialog_title_share_walking_routes_active_pedestrian");
    view.message.text = L(@"dialog_text_share_walking_routes_active_pedestrian");
    view.image.image = [UIImage imageNamed:@"img_achive_pedestrian"];
  }
  return view;
}

#pragma mark - Actions

- (IBAction)shareButtonTap
{
  [Alohalytics logEvent:kShareEventName withValue:@"shareTap"];
  [[Statistics instance] logEvent:[NSString stringWithFormat:@"%@shareTap", kShareEventName]];
  MWMActivityViewController * shareVC = [MWMActivityViewController shareControllerForPedestrianRoutesToast];
  if (IPAD && !isIOSVersionLessThan(8))
  {
    shareVC.completionWithItemsHandler = ^(NSString * activityType, BOOL completed, NSArray * returnedItems,
                                           NSError * activityError)
    {
      [self close];
    };
  }
  else
  {
    [self close];
  }
  [shareVC presentInParentViewController:self.alertController anchorView:self.shareButton];
}

- (IBAction)doneButtonTap
{
  [Alohalytics logEvent:kShareEventName withValue:@"doneTap"];
  [[Statistics instance] logEvent:[NSString stringWithFormat:@"%@doneTap", kShareEventName]];
  [self close];
}

- (void)rotate:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
  // Overriden implemantation left empty since this view is added to the view controller handling device rotation
}

- (void)addControllerViewToWindow
{
  // Overriden implemantation left empty to let sharing view appear above
}

@end
