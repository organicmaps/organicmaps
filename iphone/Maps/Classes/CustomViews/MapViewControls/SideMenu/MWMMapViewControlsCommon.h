//
//  MWMSideMenuCommon.h
//  Maps
//
//  Created by Ilya Grechuhin on 24.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

static NSTimeInterval const kMenuViewHideFramesCount = 7.0;

static inline NSTimeInterval framesDuration(NSTimeInterval const framesCount)
{
  static NSTimeInterval const kFPS = 30.0;
  static NSTimeInterval const kFrameDuration = 1.0 / kFPS;
  return kFrameDuration * framesCount;
}

static CGFloat const kViewControlsOffsetToBounds = 4.0;
