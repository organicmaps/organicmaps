#import "ColorPickerView.h"
#import "CircleView.h"
#import <QuartzCore/QuartzCore.h>

#include "Framework.h"

#define BUTTONMARGIN 7
#define BUTTONMARGINHEIGHT 16
#define BORDERMARGIN 32
#define GLOBALMARGIN 22
#define HEADERHEIGHT 56
#define LINEHEIGHT 2

struct Tcolor
{
  NSString * color;
  float rgb[3];
};

static Tcolor const g_color [] =
{
  {@"placemark-red", {255, 51, 51}},
  {@"placemark-yellow", {255, 255, 51}},
  {@"placemark-blue", {51, 204, 255}},
  {@"placemark-green", {102, 255, 51}},
  {@"placemark-purple", {153, 51, 255}},
  {@"placemark-orange", {255, 102, 0}},
  {@"placemark-brown", {102, 51, 0}},
  {@"placemark-pink", {255, 51, 255}},
};

@implementation ColorPickerView

- (id)initWithWidth:(CGFloat)width andSelectButton:(size_t)selectedIndex
{
  CGFloat const customWidth = width - 2 * GLOBALMARGIN;
  CGFloat const buttonDiameter = (customWidth - 3 * BUTTONMARGIN - 2 * BORDERMARGIN) / 4;
  self = [super initWithFrame:CGRectMake(0, 0, customWidth, 2 * (BORDERMARGIN + buttonDiameter) + LINEHEIGHT + BUTTONMARGINHEIGHT + HEADERHEIGHT)];
  self.backgroundColor = [UIColor colorWithRed:245/255.f green:245/255.f blue:245/255.f alpha:1.f];
  if (self)
  {
    self.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin|UIViewAutoresizingFlexibleRightMargin|
                            UIViewAutoresizingFlexibleTopMargin|UIViewAutoresizingFlexibleBottomMargin;

    UILabel * header = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, customWidth, HEADERHEIGHT)];
    header.backgroundColor = UIColor.clearColor;
    header.text = L(@"bookmark_color");
    header.font = [UIFont fontWithName:@"Helvetica" size:20];
    header.textAlignment = NSTextAlignmentCenter;
    header.textColor = [UIColor colorWithRed:51/255.f green:204/255.f blue:255/255.f alpha:1];

    [self addSubview:header];

    UIView * line = [[UIView alloc] initWithFrame:CGRectMake(0, HEADERHEIGHT - LINEHEIGHT, customWidth, LINEHEIGHT)];
    line.backgroundColor = [UIColor colorWithRed:51/255.f green:204/255.f blue:255/255.f alpha:1];
    [self addSubview:line];

    self.layer.cornerRadius = 10;
    for (size_t i = 0; i < 8; ++i)
    {
      UIButton * button = [[UIButton alloc] initWithFrame:CGRectMake(BORDERMARGIN + (i % 4) * (buttonDiameter + BUTTONMARGIN), BORDERMARGIN + (i / 4) * (buttonDiameter + BUTTONMARGINHEIGHT) + HEADERHEIGHT + LINEHEIGHT, buttonDiameter, buttonDiameter)];
      UIColor * c = [[self class] buttonColor:i];
      if (i != selectedIndex)
        [button setBackgroundImage:[CircleView createCircleImageWith:buttonDiameter andColor:c] forState:UIControlStateNormal];
      else
      {
        CGFloat const selectionDiametr = buttonDiameter * 0.6;
        CGFloat const origin = buttonDiameter / 2 - selectionDiametr / 2;
        UIColor * col = [UIColor colorWithRed:1.f green:1.f blue:1.f alpha:1];
        CircleView * selectedCircle = [[CircleView alloc] initWithFrame:CGRectMake(origin, origin, selectionDiametr, selectionDiametr) andColor:col];
        [button setBackgroundImage:[CircleView createCircleImageWith:buttonDiameter andColor:c andSubview:selectedCircle] forState:UIControlStateNormal];
      }

      button.layer.cornerRadius = 26;
      [button addTarget:self
                 action:@selector(touch:)
       forControlEvents:UIControlEventTouchUpInside];
      button.tag = i;
      [button setContentMode:UIViewContentModeScaleAspectFit];
      [self addSubview:button];
    }
  }
  return self;
}

- (void)touch:(UIButton *)button
{
  [self.delegate colorPicked:button.tag];
}

+ (UIColor *)buttonColor:(size_t)index
{
  return [UIColor colorWithRed:g_color[index].rgb[0]/255.f green:g_color[index].rgb[1]/255.f blue:g_color[index].rgb[2]/255.f alpha:0.8];
}


//store here temporary
+ (UIColor *)colorForName:(NSString *)name
{
  size_t const index = [self getColorIndex:name];
  return [self buttonColor:index];
}

+ (NSString *)colorName:(size_t)index
{
  if (index < ARRAY_SIZE(g_color))
    return g_color[index].color;
  NSLog(@"WARNING! Color doesn't exist");
  return @"";
}

+ (size_t)getColorIndex:(NSString *)name
{
  for (size_t i = 0; i < ARRAY_SIZE(g_color); ++i)
    if ([name isEqualToString:g_color[i].color])
      return i;
  return 0;
}

@end
