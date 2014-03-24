
#import "ContextViews.h"
#import "UIKitCategories.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "Framework.h"
#include "../../../map/measurement_utils.hpp"

@implementation LocationImageView

- (instancetype)initWithImage:(UIImage *)image
{
  self = [super initWithImage:image];

  self.userInteractionEnabled = YES;
  self.distanceLabel.hidden = YES;
  self.distanceValueLabel.hidden = YES;

  return self;
}

- (NSString *)distanceTitle
{
  if (self.userLocation)
  {
    double userLatitude = self.userLocation.coordinate.latitude;
    double userLongitude = self.userLocation.coordinate.longitude;
    double azimut = -1;
    double north = -1;

    [[MapsAppDelegate theApp].m_locationManager getNorthRad:north];

    string distance;
    GetFramework().GetDistanceAndAzimut(self.pinPoint, userLatitude, userLongitude, north, distance, azimut);
    return [NSString stringWithUTF8String:distance.c_str()];
  }
  return nil;
}

- (void)setUserLocation:(CLLocation *)userLocation
{
  if (userLocation)
  {
    _userLocation = userLocation;
    self.distanceValueLabel.text = [self distanceTitle];
    if (self.distanceLabel.hidden)
      [self layoutSubviews];
  }
}

- (void)setPinPoint:(m2::PointD)pinPoint
{
  _pinPoint = pinPoint;

  double const longitude = MercatorBounds::XToLon(self.pinPoint.x);
  self.longitudeValueLabel.text = [self coordinateString:longitude digitsCount:4];
  [self.longitudeValueLabel sizeToFit];

  double const latitude = MercatorBounds::YToLat(self.pinPoint.y);
  self.latitudeValueLabel.text = [self coordinateString:latitude digitsCount:4];
  [self.latitudeValueLabel sizeToFit];

  [self layoutSubviews];
}

- (void)layoutSubviews
{
  CGFloat const deltaY = 29;
  CGFloat const xShift = 14;
  CGFloat const yShift = 11;

  CGFloat height = yShift;

  NSString * distanceTitle = [self distanceTitle];
  if (distanceTitle)
  {
    self.distanceLabel.hidden = NO;
    self.distanceValueLabel.hidden = NO;
    self.distanceValueLabel.text = distanceTitle;

    self.distanceLabel.origin = CGPointMake(xShift, height);
    self.distanceValueLabel.maxX = self.width - xShift;
    self.distanceValueLabel.midY = self.distanceLabel.midY;

    height += deltaY;
  }
  else
  {
    self.distanceLabel.hidden = YES;
    self.distanceValueLabel.hidden = YES;
  }

  self.coordinatesLabel.origin = CGPointMake(xShift, height);

  self.latitudeValueLabel.maxX = self.width - xShift;
  self.latitudeValueLabel.midY = self.coordinatesLabel.midY;

  self.longitudeValueLabel.maxX = self.latitudeValueLabel.minX - 12;
  self.longitudeValueLabel.midY = self.coordinatesLabel.midY;

  self.height = height + deltaY + 1;
}

- (NSString *)coordinateString:(double)coordinate digitsCount:(NSInteger)digitsCount
{
  NSNumberFormatter * numberFormatter = [[NSNumberFormatter alloc] init];
  [numberFormatter setMaximumFractionDigits:digitsCount];
  [numberFormatter setMinimumFractionDigits:digitsCount];
  return [numberFormatter stringFromNumber:@(coordinate)];
}

- (BOOL)canBecomeFirstResponder
{
  return YES;
}

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender
{
  return action == @selector(copyDegreesLocation:) || action == @selector(copyDecimalLocation:);
}

- (void)copyDecimalLocation:(id)sender
{
  double const longitude = MercatorBounds::XToLon(self.pinPoint.x);
  double const latitude = MercatorBounds::YToLat(self.pinPoint.y);
  NSString * coordinates = [NSString stringWithFormat:@"%@ %@", [self coordinateString:longitude digitsCount:6], [self coordinateString:latitude digitsCount:6]];
  [UIPasteboard generalPasteboard].string = coordinates;
}

- (void)copyDegreesLocation:(id)sender
{
  double const latitude = MercatorBounds::YToLat(self.pinPoint.y);
  double const longitude = MercatorBounds::XToLon(self.pinPoint.x);
  string const coordinates = MeasurementUtils::FormatLatLonAsDMS(latitude, longitude, 3);
  [UIPasteboard generalPasteboard].string = [NSString stringWithUTF8String:coordinates.c_str()];
}

- (UILabel *)longitudeValueLabel
{
  if (!_longitudeValueLabel)
    _longitudeValueLabel = [self rightLabel];
  return _longitudeValueLabel;
}

- (UILabel *)latitudeValueLabel
{
  if (!_latitudeValueLabel)
    _latitudeValueLabel = [self rightLabel];
  return _latitudeValueLabel;
}

- (UILabel *)distanceValueLabel
{
  if (!_distanceValueLabel)
    _distanceValueLabel = [self rightLabel];
  return _distanceValueLabel;
}

- (UILabel *)coordinatesLabel
{
  if (!_coordinatesLabel)
    _coordinatesLabel = [self leftLabelWithTitle:NSLocalizedString(@"placepage_coordinates", nil)];
  return _coordinatesLabel;
}

- (UILabel *)distanceLabel
{
  if (!_distanceLabel)
    _distanceLabel = [self leftLabelWithTitle:NSLocalizedString(@"placepage_distance", nil)];
  return _distanceLabel;
}

- (UILabel *)rightLabel
{
  UILabel * label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 74, 18)];
  label.backgroundColor = [UIColor clearColor];
  label.font = [UIFont fontWithName:@"HelveticaNeue" size:15];
  label.textAlignment = NSTextAlignmentRight;
  label.textColor = [UIColor colorWithColorCode:@"4cd964"];
  label.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
  [self addSubview:label];

  return label;
}

- (UILabel *)leftLabelWithTitle:(NSString *)title
{
  UILabel * label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 120, 18)];
  label.backgroundColor = [UIColor clearColor];
  label.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:15];
  label.textColor = [UIColor colorWithColorCode:@"333333"];
  label.autoresizingMask = UIViewAutoresizingFlexibleRightMargin;
  label.text = title;
  [self addSubview:label];

  return label;
}

@end


@implementation CopyLabel

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];

  self.userInteractionEnabled = YES;
  UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tap:)];
  [self addGestureRecognizer:tap];

  return self;
}

- (BOOL)canBecomeFirstResponder
{
  return YES;
}

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender
{
  return action == @selector(copy:);
}

- (void)copy:(id)sender
{
  [UIPasteboard generalPasteboard].string = self.text;
}

@end
