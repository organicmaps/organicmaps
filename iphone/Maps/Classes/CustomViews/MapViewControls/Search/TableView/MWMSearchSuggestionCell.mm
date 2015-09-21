#import "MWMSearchSuggestionCell.h"
#import "UIColor+MapsMeColor.h"
#import "UIFont+MapsMeFonts.h"

static NSDictionary * const kSelectedAttributes =
    @{NSForegroundColorAttributeName : UIColor.linkBlue, NSFontAttributeName : UIFont.bold16};

static NSDictionary * const kUnelectedAttributes =
    @{NSForegroundColorAttributeName : UIColor.linkBlue, NSFontAttributeName : UIFont.regular16};

@interface MWMSearchSuggestionCell ()

@property (weak, nonatomic) IBOutlet UIImageView * icon;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * separatorLeftOffset;

@end

@implementation MWMSearchSuggestionCell

- (NSDictionary *)selectedTitleAttributes
{
  return kSelectedAttributes;
}

- (NSDictionary *)unselectedTitleAttributes
{
  return kUnelectedAttributes;
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
