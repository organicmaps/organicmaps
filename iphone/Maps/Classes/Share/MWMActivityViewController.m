//
//  MWMActivityViewController.m
//  Maps
//
//  Created by Ilya Grechuhin on 05.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMActivityViewController.h"
#import "MWMShareLocationActivityItem.h"

@interface MWMActivityViewController ()

@property (weak, nonatomic) UIViewController * ownerViewController;
@property (weak, nonatomic) UIView * anchorView;

@end

@implementation MWMActivityViewController

- (instancetype)initWithActivityItem:(id<UIActivityItemSource>)activityItem
{
  self = [super initWithActivityItems:@[activityItem] applicationActivities:nil];
  if (self)
    self.excludedActivityTypes = @[UIActivityTypePrint, UIActivityTypeAssignToContact, UIActivityTypeSaveToCameraRoll,
                                   UIActivityTypeAirDrop, UIActivityTypeAddToReadingList, UIActivityTypePostToFlickr,
                                   UIActivityTypePostToVimeo];
  return self;
}

+ (instancetype)shareControllerForLocationTitle:(NSString *)title location:(CLLocationCoordinate2D)location
                                     myPosition:(BOOL)myPosition
{
  MWMShareLocationActivityItem * item = [[MWMShareLocationActivityItem alloc] initWithTitle:title location:location
                                                                                 myPosition:myPosition];
  return [[self alloc] initWithActivityItem:item];
}

- (void)presentInParentViewController:(UIViewController *)parentVC anchorView:(UIView *)anchorView
{
  self.ownerViewController = parentVC;
  self.anchorView = anchorView;
  if ([self respondsToSelector:@selector(popoverPresentationController)])
  {
    self.popoverPresentationController.sourceView = anchorView;
    self.popoverPresentationController.sourceRect = anchorView.bounds;
  }
  [parentVC presentViewController:self animated:YES completion:nil];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [self dismissViewControllerAnimated:YES completion:nil];
  [coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {}
                              completion:^(id<UIViewControllerTransitionCoordinatorContext> context)
  {
    [self presentInParentViewController:self.ownerViewController anchorView:self.anchorView];
  }];
}

@end
