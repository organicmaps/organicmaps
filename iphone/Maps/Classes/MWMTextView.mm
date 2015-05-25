//
//  MWMTextView.m
//  Maps
//
//  Created by v.mikhaylenko on 24.05.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMTextView.h"
#import "Common.h"

static CGFloat const kDefaultTextLeftInset = 5.;

@interface MWMTextView ()

@property (nonatomic) UILabel *placeholderView;

@end

@implementation MWMTextView

- (instancetype)initWithCoder:(NSCoder *)coder
{
  self = [super initWithCoder:coder];
  if (self)
    [self preparePlaceholder];

  return self;
}

- (instancetype)initWithFrame:(CGRect)frame textContainer:(NSTextContainer *)textContainer
{
  self = [super initWithFrame:frame textContainer:textContainer];
  if (self)
    [self preparePlaceholder];

  return self;
}

- (void)preparePlaceholder
{
  NSAssert(!self.placeholderView, @"placeholder has been prepared already: %@", self.placeholderView);

  self.placeholderView = [[UILabel alloc] initWithFrame:self.bounds];
  self.placeholderView.opaque = NO;
  self.placeholderView.backgroundColor = [UIColor clearColor];
  self.placeholderView.textColor = [UIColor lightGrayColor];
  self.placeholderView.textAlignment = self.textAlignment;
  self.placeholderView.userInteractionEnabled = NO;
  self.placeholderView.font = self.font;
  self.placeholderView.isAccessibilityElement = NO;

  if (!isIOSVersionLessThan(7))
    [self setTextContainerInset:UIEdgeInsetsZero];

  [self updatePlaceholderVisibility];

  NSNotificationCenter * defaultCenter = [NSNotificationCenter defaultCenter];
  [defaultCenter addObserver:self selector:@selector(textDidChange:)
                        name:UITextViewTextDidChangeNotification object:self];
  self.clipsToBounds = YES;
}

#pragma mark - Setters

- (void)setPlaceholder:(NSString *)placeholder
{
  _placeholder = placeholder.copy;
  self.placeholderView.text = placeholder;
  [self resizePlaceholderFrame];
}

- (void)setFont:(UIFont *)font
{
  [super setFont:font];
  self.placeholderView.font = font;
}

- (void)setAttributedText:(NSAttributedString *)attributedText
{
  [super setAttributedText:attributedText];
  [self updatePlaceholderVisibility];
}

- (void)setText:(NSString *)text
{
  [super setText:text];
  [self updatePlaceholderVisibility];
}

- (void)setTextAlignment:(NSTextAlignment)textAlignment
{
  [super setTextAlignment:textAlignment];
  self.placeholderView.textAlignment = textAlignment;
}

- (void)setTextContainerInset:(UIEdgeInsets)textContainerInset
{
  textContainerInset.left -= kDefaultTextLeftInset;
  [super setTextContainerInset:textContainerInset];
  [self updatePlaceholderInset:textContainerInset];
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  [self resizePlaceholderFrame];
}

- (void)resizePlaceholderFrame
{
  self.placeholderView.numberOfLines = 0;
  [self.placeholderView sizeToFit];
}

- (void)textDidChange:(NSNotification *)aNotification
{
  [self updatePlaceholderVisibility];
}

- (void)updatePlaceholderInset:(UIEdgeInsets)inset
{
  self.placeholderView.frame = CGRectMake(inset.left + kDefaultTextLeftInset, inset.top, self.bounds.size.width - inset.right, self.bounds.size.height - inset.bottom);
  [self resizePlaceholderFrame];
}

- (void)updatePlaceholderVisibility
{
  if (self.text.length == 0) {
    [self addSubview:self.placeholderView];
    [self sendSubviewToBack:self.placeholderView];
  }
  else
  {
    [self.placeholderView removeFromSuperview];
  }
}

@end
