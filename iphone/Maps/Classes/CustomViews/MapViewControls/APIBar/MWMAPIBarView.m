//
//  MWMAPIBarView.m
//  Maps
//
//  Created by Ilya Grechuhin on 27.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAPIBarView.h"
#import "UIKitCategories.h"

@interface MWMAPIBarView ()

@property (nonatomic) CGFloat defaulhHeight;

@end

@implementation MWMAPIBarView

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  return self;
}

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.defaulhHeight = self.height;
  self.targetY = 0.0;
}

- (void)layoutSubviews
{
  self.width = self.superview.width;
  self.height = self.defaulhHeight;
  self.minX = 0.0;
  self.minY = self.targetY;
  [super layoutSubviews];
}

#pragma mark - Properties

- (void)setTargetY:(CGFloat)targetY
{
  _targetY = targetY;
  [self layoutSubviews];
}

@end
