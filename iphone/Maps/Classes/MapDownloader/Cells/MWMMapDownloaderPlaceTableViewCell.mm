#import "Common.h"
#import "MWMMapDownloaderPlaceTableViewCell.h"
#import "UIFont+MapsMeFonts.h"

#include "Framework.h"

namespace
{
  NSDictionary * const kSelectedAreaAttrs = @{ NSFontAttributeName : [UIFont bold12] };
  NSDictionary * const kUnselectedAreaAttrs = @{ NSFontAttributeName : [UIFont regular12] };
} // namespace

@interface MWMMapDownloaderTableViewCell ()

- (NSAttributedString *)matchedString:(NSString *)str selectedAttrs:(NSDictionary *)selectedAttrs unselectedAttrs:(NSDictionary *)unselectedAttrs;

@end

@interface MWMMapDownloaderPlaceTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * area;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleBottomOffset;

@end

@implementation MWMMapDownloaderPlaceTableViewCell

+ (CGFloat)estimatedHeight
{
  return 62.0;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  if (isIOS7)
  {
    self.area.preferredMaxLayoutWidth = self.area.width;
    [super layoutSubviews];
  }
}

#pragma mark - Config

- (void)config:(storage::NodeAttrs const &)nodeAttrs
{
  [super config:nodeAttrs];
  BOOL isAreaVisible = NO;
  if (self.needDisplayArea && nodeAttrs.m_topmostParentInfo.size() == 1)
  {
    string const & areaName = nodeAttrs.m_topmostParentInfo[0].m_localName;
    isAreaVisible = (areaName != GetFramework().Storage().GetRootId());
    if (isAreaVisible)
      self.area.attributedText = [self matchedString:@(areaName.c_str())
                                       selectedAttrs:kSelectedAreaAttrs
                                     unselectedAttrs:kUnselectedAreaAttrs];
  }
  self.area.hidden = !isAreaVisible;
  self.titleBottomOffset.priority = isAreaVisible ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
}

@end
