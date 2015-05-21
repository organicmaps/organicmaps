//
//  MWMPlacePageBaseCell.m
//  Maps
//
//  Created by v.mikhaylenko on 27.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMPlacePageInfoCell.h"
#import "UIKitCategories.h"

@interface MWMPlacePageInfoCell () <UITextViewDelegate>

@property (weak, nonatomic, readwrite) IBOutlet UIImageView * icon;
@property (weak, nonatomic, readwrite) IBOutlet id textContainer;

@end

@implementation MWMPlacePageInfoCell

- (void)configureWithIconTitle:(NSString *)title info:(NSString *)info
{
  [self.imageView setImage:[UIImage imageNamed:[NSString stringWithFormat:@"ic_%@", title]]];
  [self.textContainer setText:info];

#warning TODO(Vlad): change 0.7f to computational constant.
  [(UIView *)self.textContainer setWidth:self.bounds.size.width * .7f];

  if ([self.textContainer respondsToSelector:@selector(sizeToFit)])
      [self.textContainer sizeToFit];
}

- (BOOL)textView:(UITextView *)textView shouldInteractWithURL:(NSURL *)URL inRange:(NSRange)characterRange
{
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString:textView.text]];
  return YES;
}
@end
