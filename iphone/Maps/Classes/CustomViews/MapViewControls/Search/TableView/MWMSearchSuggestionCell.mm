#import "MWMSearchSuggestionCell.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"

static NSDictionary * const selectedAttributes =
    @{NSForegroundColorAttributeName : UIColor.linkBlue, NSFontAttributeName : UIFont.bold16};

static NSDictionary * const unselectedAttributes =
    @{NSForegroundColorAttributeName : UIColor.linkBlue, NSFontAttributeName : UIFont.regular16};

@interface MWMSearchSuggestionCell ()

@property (weak, nonatomic) IBOutlet UIImageView * icon;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * separatorLeftOffset;

@end

@implementation MWMSearchSuggestionCell

- (NSDictionary *)selectedTitleAttributes
{
  return selectedAttributes;
}

- (NSDictionary *)unselectedTitleAttributes
{
  return unselectedAttributes;
}

+ (CGFloat)cellHeight
{
  return 44.0;
}

#pragma mark - Properties

- (void)setIsLightTheme:(BOOL)isLightTheme
{
  _isLightTheme = isLightTheme;
  self.icon.image = [UIImage imageNamed:isLightTheme ? @"ic_search_suggest_light" : @"ic_search_suggest_dark"];
}

- (void)setIsLastCell:(BOOL)isLastCell
{
  _isLastCell = isLastCell;
  self.separatorLeftOffset.constant = isLastCell ? 0.0 : 60.0;
}

@end
