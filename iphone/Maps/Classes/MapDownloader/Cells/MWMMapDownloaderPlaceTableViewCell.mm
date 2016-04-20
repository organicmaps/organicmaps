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

@property (weak, nonatomic) IBOutlet UILabel * descriptionLabel;
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
    self.descriptionLabel.preferredMaxLayoutWidth = self.descriptionLabel.width;
    [super layoutSubviews];
  }
}

#pragma mark - Config

- (void)config:(storage::NodeAttrs const &)nodeAttrs
{
  [super config:nodeAttrs];
  BOOL isDescriptionVisible = NO;
  if (self.needDisplayArea && nodeAttrs.m_topmostParentInfo.size() == 1)
  {
    string const & areaName = nodeAttrs.m_topmostParentInfo[0].m_localName;
    isDescriptionVisible = (areaName != GetFramework().Storage().GetRootId());
    if (isDescriptionVisible)
      self.descriptionLabel.attributedText = [self matchedString:@(areaName.c_str())
                                                   selectedAttrs:kSelectedAreaAttrs
                                                 unselectedAttrs:kUnselectedAreaAttrs];
  }
  else if (!nodeAttrs.m_nodeLocalDescription.empty())
  {
    isDescriptionVisible = YES;
    self.descriptionLabel.attributedText = [self matchedString:@(nodeAttrs.m_nodeLocalDescription.c_str())
                                                 selectedAttrs:kSelectedAreaAttrs
                                               unselectedAttrs:kUnselectedAreaAttrs];
  }
  self.descriptionLabel.hidden = !isDescriptionVisible;
  self.titleBottomOffset.priority = isDescriptionVisible ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
}

@end
