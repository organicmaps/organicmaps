typedef NS_ENUM(NSUInteger, MWMButtonColoring)
{
  MWMButtonColoringOther,
  MWMButtonColoringBlue,
  MWMButtonColoringBlack,
  MWMButtonColoringWhite,
  MWMButtonColoringWhiteText,
  MWMButtonColoringGray
};

@interface MWMButton : UIButton

@property (copy, nonatomic) NSString * imageName;
@property (nonatomic) MWMButtonColoring coloring;

@end
