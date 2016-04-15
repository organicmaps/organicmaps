typedef NS_ENUM(NSUInteger, MWMImageColoring)
{
  MWMImageColoringOther,
  MWMImageColoringBlue,
  MWMImageColoringBlack,
  MWMImageColoringGray,
  MWMImageColoringSeparator
};

@interface UIImageView (Coloring)

@property (nonatomic) MWMImageColoring mwm_coloring;
@property (copy, nonatomic) NSString * mwm_name;

- (void)changeColoringToOpposite;

@end
