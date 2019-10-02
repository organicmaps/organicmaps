#import "MWMMapDownloaderSubplaceTableViewCell.h"

@interface MWMMapDownloaderTableViewCell ()

- (NSAttributedString *)matchedString:(NSString *)str
                        selectedAttrs:(NSDictionary *)selectedAttrs
                      unselectedAttrs:(NSDictionary *)unselectedAttrs;

@end

@interface MWMMapDownloaderSubplaceTableViewCell ()

@property(weak, nonatomic) IBOutlet UILabel * subPlace;

@end

@implementation MWMMapDownloaderSubplaceTableViewCell

+ (CGFloat)estimatedHeight { return 82.0; }

- (void)setSubplaceText:(NSString *)text
{
  NSDictionary * const selectedSubPlaceAttrs = @{NSFontAttributeName : [UIFont bold14]};
  NSDictionary * const unselectedSubPlaceAttrs = @{NSFontAttributeName : [UIFont regular14]};
  self.subPlace.attributedText = [self matchedString:text
                                       selectedAttrs:selectedSubPlaceAttrs
                                     unselectedAttrs:unselectedSubPlaceAttrs];
}

@end
