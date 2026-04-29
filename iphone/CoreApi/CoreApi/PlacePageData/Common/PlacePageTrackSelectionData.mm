#import "PlacePageTrackSelectionData+Core.h"

@interface PlacePageTrackSelectionData ()
{
  FeatureID _relationId;
}

@property(nonatomic, readwrite) MWMTrackID trackId;
@property(nonatomic, readwrite) NSString * title;
@property(nonatomic, readwrite) UIColor * color;
@property(nonatomic, readwrite) BOOL isSelected;

@end

@implementation PlacePageTrackSelectionData

- (instancetype)initWithTrackId:(MWMTrackID)trackId
                     relationId:(FeatureID)relationId
                          title:(NSString *)title
                          color:(UIColor *)color
                     isSelected:(BOOL)isSelected
{
  self = [super init];
  if (self)
  {
    _trackId = trackId;
    _relationId = relationId;
    _title = title;
    _color = color;
    _isSelected = isSelected;
  }
  return self;
}

- (FeatureID)relationId
{
  return _relationId;
}

@end
