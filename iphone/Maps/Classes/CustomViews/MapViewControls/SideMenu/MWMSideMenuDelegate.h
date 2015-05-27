//
//  MWMSideMenuDelegate.h
//  Maps
//
//  Created by Ilya Grechuhin on 27.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#include "geometry/point2d.hpp"

@protocol MWMSideMenuInformationDisplayProtocol <NSObject>

- (void)setRulerPivot:(m2::PointD)pivot;
- (void)setCopyrightLabelPivot:(m2::PointD)pivot;

@end
