//
//  MWMRoutePreview.h
//  Maps
//
//  Created by Ilya Grechuhin on 21.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMNavigationView.h"

@interface MWMRoutePreview : MWMNavigationView

@property (nonatomic) BOOL showGoButton;

@property (weak, nonatomic) IBOutlet UILabel * status;
@property (weak, nonatomic) IBOutlet UIButton * pedestrian;
@property (weak, nonatomic) IBOutlet UIButton * vehicle;

@end
