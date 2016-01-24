#import "MWMPlacePageEntity.h"

@interface MWMPlacePageTypeDescriptionView : UIView

- (void)layoutNearPoint:(CGPoint const &)point;

@end

@interface MWMPlacePageTypeDescription : NSObject

@property (strong, nonatomic) IBOutlet MWMPlacePageTypeDescriptionView * eleDescription;
@property (strong, nonatomic) IBOutlet MWMPlacePageTypeDescriptionView * hotelDescription;

- (instancetype)initWithPlacePageEntity:(MWMPlacePageEntity *)entity;

@end
