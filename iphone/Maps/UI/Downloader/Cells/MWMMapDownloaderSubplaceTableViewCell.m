#import "MWMMapDownloaderSubplaceTableViewCell.h"

@interface MWMMapDownloaderSubplaceTableViewCell ()

@property(weak, nonatomic) IBOutlet UILabel *subPlace;

@end

@implementation MWMMapDownloaderSubplaceTableViewCell

- (void)setSubplaceText:(NSString *)text {
  self.subPlace.attributedText = [self matchedString:text
                                       selectedAttrs:@{NSFontAttributeName : [UIFont bold14]}
                                     unselectedAttrs:@{NSFontAttributeName : [UIFont regular14]}];
}

@end
