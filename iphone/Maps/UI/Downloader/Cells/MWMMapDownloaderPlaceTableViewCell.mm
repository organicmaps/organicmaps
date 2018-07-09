#import "MWMMapDownloaderPlaceTableViewCell.h"

#include "Framework.h"

@interface MWMMapDownloaderTableViewCell ()

- (NSAttributedString *)matchedString:(NSString *)str
                        selectedAttrs:(NSDictionary *)selectedAttrs
                      unselectedAttrs:(NSDictionary *)unselectedAttrs;

@end

@interface MWMMapDownloaderPlaceTableViewCell ()

@property(weak, nonatomic) IBOutlet UILabel * descriptionLabel;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint * titleBottomOffset;

@end

@implementation MWMMapDownloaderPlaceTableViewCell

+ (CGFloat)estimatedHeight { return 62.0; }

#pragma mark - Config

- (void)config:(storage::NodeAttrs const &)nodeAttrs
{
  [super config:nodeAttrs];
  BOOL isDescriptionVisible = NO;
  NSDictionary * const selectedAreaAttrs = @{NSFontAttributeName : [UIFont bold12]};
  NSDictionary * const unselectedAreaAttrs = @{NSFontAttributeName : [UIFont regular12]};
  if (self.needDisplayArea && nodeAttrs.m_topmostParentInfo.size() == 1)
  {
    string const & areaName = nodeAttrs.m_topmostParentInfo[0].m_localName;
    isDescriptionVisible = (areaName != GetFramework().GetStorage().GetRootId());
    if (isDescriptionVisible)
      self.descriptionLabel.attributedText = [self matchedString:@(areaName.c_str())
                                                   selectedAttrs:selectedAreaAttrs
                                                 unselectedAttrs:unselectedAreaAttrs];
  }
  else if (!nodeAttrs.m_nodeLocalDescription.empty())
  {
    isDescriptionVisible = YES;
    self.descriptionLabel.attributedText =
        [self matchedString:@(nodeAttrs.m_nodeLocalDescription.c_str())
              selectedAttrs:selectedAreaAttrs
            unselectedAttrs:unselectedAreaAttrs];
  }
  self.descriptionLabel.hidden = !isDescriptionVisible;
  self.titleBottomOffset.priority =
      isDescriptionVisible ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
}

@end
