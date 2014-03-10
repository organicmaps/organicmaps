
#import <Foundation/Foundation.h>
#include "../../geometry/point2d.hpp"

@interface LocationImageView : UIImageView

@property (nonatomic) UILabel * longitudeValueLabel;
@property (nonatomic) UILabel * latitudeValueLabel;
@property (nonatomic) UILabel * coordinatesLabel;
@property (nonatomic) UILabel * distanceLabel;
@property (nonatomic) UILabel * distanceValueLabel;

@property (nonatomic) m2::PointD pinPoint;
@property (nonatomic) CLLocation * userLocation;

@end


@interface CopyLabel : UILabel

@end