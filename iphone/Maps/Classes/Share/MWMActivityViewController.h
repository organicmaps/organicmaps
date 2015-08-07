//
//  MWMActivityViewController.h
//  Maps
//
//  Created by Ilya Grechuhin on 05.08.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

@interface MWMActivityViewController : UIActivityViewController

+ (instancetype)shareControllerForLocationTitle:(NSString *)title location:(CLLocationCoordinate2D)location
                                     myPosition:(BOOL)myPosition;
+ (instancetype)shareControllerForPedestrianRoutesToast;

- (void)presentInParentViewController:(UIViewController *)parentVC anchorView:(UIView *)anchorView;

@end
