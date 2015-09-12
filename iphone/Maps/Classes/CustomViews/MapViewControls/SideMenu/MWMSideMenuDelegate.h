#include "geometry/point2d.hpp"

@protocol MWMSideMenuInformationDisplayProtocol <NSObject>

- (void)setRulerPivot:(m2::PointD)pivot;
- (void)setCopyrightLabelPivot:(m2::PointD)pivot;

@end
