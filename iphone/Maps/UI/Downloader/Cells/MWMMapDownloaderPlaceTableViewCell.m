#import "MWMMapDownloaderPlaceTableViewCell.h"

#import <CoreApi/MWMMapNodeAttributes.h>

@interface MWMMapDownloaderPlaceTableViewCell ()

@property(weak, nonatomic) IBOutlet UILabel *descriptionLabel;
@property(weak, nonatomic) IBOutlet NSLayoutConstraint *titleBottomOffset;

@end

@implementation MWMMapDownloaderPlaceTableViewCell

#pragma mark - Config

- (void)config:(MWMMapNodeAttributes *)nodeAttrs searchQuery:(NSString *)searchQuery {
  [super config:nodeAttrs searchQuery:searchQuery];
  BOOL isDescriptionVisible = NO;
  NSDictionary *selectedAreaAttrs = @{NSFontAttributeName : [UIFont bold12]};
  NSDictionary *unselectedAreaAttrs = @{NSFontAttributeName : [UIFont regular12]};
  self.needDisplayArea = !nodeAttrs.hasParent;
  if (self.needDisplayArea && nodeAttrs.topmostParentInfo.count == 1) {
    isDescriptionVisible = nodeAttrs.hasParent;
    if (isDescriptionVisible) {
      self.descriptionLabel.attributedText = [self matchedString:nodeAttrs.topmostParentInfo[0].countryName
                                                   selectedAttrs:selectedAreaAttrs
                                                 unselectedAttrs:unselectedAreaAttrs];
    }
  }
  else if (nodeAttrs.nodeDescription.length > 0)
  {
    isDescriptionVisible = YES;
    self.descriptionLabel.attributedText = [self matchedString:nodeAttrs.nodeDescription
                                                 selectedAttrs:selectedAreaAttrs
                                               unselectedAttrs:unselectedAreaAttrs];
  }
  self.descriptionLabel.hidden = !isDescriptionVisible;
  self.titleBottomOffset.priority =
      isDescriptionVisible ? UILayoutPriorityDefaultLow : UILayoutPriorityDefaultHigh;
}

@end
