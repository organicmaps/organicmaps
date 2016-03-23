#import "Common.h"
#import "MWMMapDownloaderSubplaceTableViewCell.h"
#import "UIFont+MapsMeFonts.h"

namespace
{
  NSDictionary * const kSelectedSubPlaceAttrs = @{ NSFontAttributeName : [UIFont bold14] };
  NSDictionary * const kUnselectedSubPlaceAttrs = @{ NSFontAttributeName : [UIFont regular14] };
} // namespace

@interface MWMMapDownloaderTableViewCell ()

- (NSAttributedString *)matchedString:(NSString *)str selectedAttrs:(NSDictionary *)selectedAttrs unselectedAttrs:(NSDictionary *)unselectedAttrs;

@end

@interface MWMMapDownloaderSubplaceTableViewCell ()

@property (weak, nonatomic) IBOutlet UILabel * subPlace;

@end

@implementation MWMMapDownloaderSubplaceTableViewCell

+ (CGFloat)estimatedHeight
{
  return 82.0;
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  if (isIOS7)
  {
    self.subPlace.preferredMaxLayoutWidth = floor(self.subPlace.width);
    [super layoutSubviews];
  }
}

- (void)setSubplaceText:(NSString *)text
{
  self.subPlace.attributedText = [self matchedString:text
                                       selectedAttrs:kSelectedSubPlaceAttrs
                                     unselectedAttrs:kUnselectedSubPlaceAttrs];
}

@end
