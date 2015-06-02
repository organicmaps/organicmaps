//
//  MWMMapViewControlsWrapper.mm
//  Maps
//
//  Created by Ilya Grechuhin on 02.06.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMMapViewControlsWrapper.h"

@implementation MWMMapViewControlsWrapper

- (instancetype)initWithParentView:(UIView *)view
{
  self = [super initWithFrame:view.bounds];
  if (!self)
    return nil;
  [view addSubview:self];
  self.contentScaleFactor = view.contentScaleFactor;
  self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.frame = self.superview.bounds;
}

@end
