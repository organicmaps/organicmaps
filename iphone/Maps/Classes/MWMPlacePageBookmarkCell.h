#import "MWMTableViewCell.h"

@protocol MWMPlacePageBookmarkDelegate <NSObject>

- (void)reloadBookmark;
- (void)editBookmarkTap;
- (void)moreTap;

@end

@class MWMPlacePage;

@interface MWMPlacePageBookmarkCell : MWMTableViewCell

- (void)configWithText:(NSString *)text
              delegate:(id<MWMPlacePageBookmarkDelegate>)delegate
        placePageWidth:(CGFloat)width
                isOpen:(BOOL)isOpen
                isHtml:(BOOL)isHtml;

- (CGFloat)cellHeight;

@end
