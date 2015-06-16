//
//  MWMBookmarkColorCell.m
//  Maps
//
//  Created by v.mikhaylenko on 27.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMBookmarkColorCell.h"
#import "UIKitCategories.h"

extern NSArray * const kBookmarkColorsVariant;

@interface MWMBookmarkColorCell ()

@property (copy, nonatomic) NSString * currentImageName;

@end

@implementation MWMBookmarkColorCell

- (void)configureWithColorString:(NSString *)colorString
{
  CGFloat const leftOffset = 60.;
  CGFloat const rightOffset = 42.;
  self.currentImageName = colorString;
  self.titleLabel.text = L([colorString stringByReplacingOccurrencesOfString:@"placemark-" withString:@""]);
  self.titleLabel.width = self.bounds.size.width - leftOffset - rightOffset;
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated
{
  [super setSelected:selected animated:animated];
  [self.colorButton setImage:[UIImage imageNamed:[NSString stringWithFormat:@"%@%@", self.currentImageName, selected ? @"-on" : @"-off"]] forState:UIControlStateNormal];
  self.approveImageView.hidden = !selected;
}

@end
