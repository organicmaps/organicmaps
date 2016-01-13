typedef NS_ENUM(NSUInteger, MWMButtonColoring)
{
  MWMButtonColoringOther,
  MWMButtonColoringBlue,
  MWMButtonColoringBlack,
  MWMButtonColoringGray
};

@interface UIButton (Coloring)

@property (nonatomic) MWMButtonColoring mwm_coloring;
@property (copy, nonatomic) NSString * mwm_name;

- (void)changeColoringToOpposite;

@end
