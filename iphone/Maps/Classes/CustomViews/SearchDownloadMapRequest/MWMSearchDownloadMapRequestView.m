//
//  MWMSearchDownloadMapRequestView.m
//  Maps
//
//  Created by Ilya Grechuhin on 09.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "Macros.h"
#import "MWMDownloadMapRequestView.h"
#import "MWMSearchDownloadMapRequestView.h"
#import "UIKitCategories.h"

@interface MWMSearchDownloadMapRequestView ()

@property (weak, nonatomic) IBOutlet UILabel * hint;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * hintTopOffset;

@end

@implementation MWMSearchDownloadMapRequestView

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
  self = [super initWithCoder:aDecoder];
  if (self)
  {
    self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  }
  return self;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  UIView * superview = self.superview;
  self.frame = superview.bounds;
  // @TODO Remove on new search!
  CGFloat const someCrazyTopOfsetForCurrentSearchViewImplementation = 64.0;
  CGFloat const topOffset = (IPAD || superview.height > superview.width ? 40.0 : 12.0);
  self.hintTopOffset.constant = someCrazyTopOfsetForCurrentSearchViewImplementation + topOffset;
}

#pragma mark - Properties

- (void)setHintHidden:(BOOL)hintHidden
{
  if (self.hint.hidden == hintHidden)
    return;
  if (!hintHidden)
    self.hint.hidden = hintHidden;
  [UIView animateWithDuration:0.3 animations:^
  {
    self.hint.alpha = hintHidden ? 0.0 : 1.0;
  }
  completion:^(BOOL finished)
  {
    if (hintHidden)
      self.hint.hidden = hintHidden;
  }];
}

- (BOOL)hintHidden
{
  return self.hint.hidden;
}

@end
