
#import "PlacePageInfoCell.h"
#import "UIKitCategories.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "Framework.h"
#include "../../../platform/settings.hpp"
#include "../../../platform/measurement_utils.hpp"
#include "../../../geometry/distance_on_sphere.hpp"
#import "ContextViews.h"

#define SETTINGS_KEY_USE_DMS "UseDMSCoordinates"

@interface PlacePageInfoCell () <SelectedColorViewDelegate>

@property (nonatomic) UILabel * distanceLabel;
@property (nonatomic) CopyView * coordinatesView;
@property (nonatomic) UILabel * latitudeLabel;
@property (nonatomic) UILabel * longitudeLabel;
@property (nonatomic) SmallCompassView * compassView;
@property (nonatomic) UIImageView * separatorView;
@property (nonatomic) SelectedColorView * selectedColorView;

@end

@implementation PlacePageInfoCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  self.selectionStyle = UITableViewCellSelectionStyleNone;
  self.backgroundColor = [UIColor clearColor];

  [self addSubview:self.compassView];
  [self addSubview:self.distanceLabel];
  [self addSubview:self.coordinatesView];
  [self.coordinatesView addSubview:self.latitudeLabel];
  [self.coordinatesView addSubview:self.longitudeLabel];
  [self addSubview:self.selectedColorView];
  [self addSubview:self.separatorView];

  return self;
}

- (void)updateDistance
{
  self.distanceLabel.text = [self distance];
}

- (NSString *)distance
{
#warning тут про локацию
  CLLocation * location = [MapsAppDelegate theApp].m_locationManager.lastLocation;
  if (location)
  {
    double userLatitude = location.coordinate.latitude;
    double userLongitude = location.coordinate.longitude;
    double azimut = -1;
    double north = -1;

    [[MapsAppDelegate theApp].m_locationManager getNorthRad:north];

    string distance;
    GetFramework().GetDistanceAndAzimut(self.pinPoint, userLatitude, userLongitude, north, distance, azimut);
    return [NSString stringWithUTF8String:distance.c_str()];
  }
  return nil;
}

- (void)updateCoordinates
{
  bool useDMS = false;
  (void)Settings::Get(SETTINGS_KEY_USE_DMS, useDMS);
  m2::PointD point;
  if (self.myPositionMode)
  {
    CLLocationCoordinate2D const coordinate = [MapsAppDelegate theApp].m_locationManager.lastLocation.coordinate;
    point = MercatorBounds::FromLatLon(coordinate.latitude, coordinate.longitude);
  }
  else
  {
    point = self.pinPoint;
  }
  
  int const dmcDac = 2;
  int const dac = 6;
  string const coords = useDMS ? MeasurementUtils::FormatMercatorAsDMS(point, dmcDac)
                               : MeasurementUtils::FormatMercator(point, dac);
  self.coordinatesView.textToCopy = [NSString stringWithUTF8String:coords.c_str()];

  string lat;
  string lon;
  if (useDMS)
    MeasurementUtils::FormatMercatorAsDMS(point, lat, lon, dmcDac);
  else
    MeasurementUtils::FormatMercator(point, lat, lon, dac);

  self.latitudeLabel.text = [NSString stringWithUTF8String:lat.c_str()];
  self.longitudeLabel.text = [NSString stringWithUTF8String:lon.c_str()];
}

- (void)setPinPoint:(m2::PointD)pinPoint
{
  _pinPoint = pinPoint;
  self.distanceLabel.text = [self distance];
  [self updateCoordinates];
}

#define RIGHT_SHIFT 55
#define LEFT_SHIFT 14

- (void)layoutSubviews
{
  [self.latitudeLabel sizeToIntegralFit];
  [self.longitudeLabel sizeToIntegralFit];
  BOOL const shouldShowLocationViews = [[MapsAppDelegate theApp].m_locationManager enabledOnMap] && !self.myPositionMode;
  if (shouldShowLocationViews)
  {
    BOOL const headingAvailable = [CLLocationManager headingAvailable];
    self.compassView.origin = CGPointMake(15 - (headingAvailable ? 0 : self.compassView.width + 17), INTEGRAL(12.5));
    self.compassView.hidden = !headingAvailable;

    CGFloat const width = 134;
    self.distanceLabel.frame = CGRectMake(self.compassView.maxX + 15, 13, width, 20);
    self.distanceLabel.hidden = NO;

    self.coordinatesView.frame = CGRectMake(self.distanceLabel.minX, self.distanceLabel.maxY + 6, width, 44);

    self.latitudeLabel.origin = CGPointMake(0, 0);
    self.longitudeLabel.origin = CGPointMake(0, self.latitudeLabel.maxY);
  }
  else
  {
    self.compassView.hidden = YES;
    self.distanceLabel.hidden = YES;

    self.coordinatesView.frame = CGRectMake(LEFT_SHIFT, 5, 250, 44);

    self.latitudeLabel.minX = 0;
    self.latitudeLabel.midY = self.latitudeLabel.superview.height / 2;

    self.longitudeLabel.minX = self.latitudeLabel.maxX + 8;
    self.longitudeLabel.midY = self.longitudeLabel.superview.height / 2;
  }

  self.selectedColorView.center = CGPointMake(self.width - 24, 27);
  [self.selectedColorView setColor:self.color];

  self.separatorView.maxY = self.height;
  CGFloat const shift = 12.5;
  self.separatorView.width = self.width - 2 * shift;
  self.separatorView.minX = shift;

  self.backgroundColor = [UIColor clearColor];
}

+ (CGFloat)cellHeightWithViewWidth:(CGFloat)viewWidth inMyPositionMode:(BOOL)myPositon
{
  BOOL const shouldShowLocationViews = [[MapsAppDelegate theApp].m_locationManager enabledOnMap] && !myPositon;
  return shouldShowLocationViews ? 95 : 55;
}

- (void)addressPress:(id)sender
{
  [self.delegate infoCellDidPressAddress:self withGestureRecognizer:sender];
}

// Change displayed coordinates system from decimal to DMS and back on single tap
- (void)coordinatesTap:(id)sender
{
  bool useDMS = false;
  (void)Settings::Get(SETTINGS_KEY_USE_DMS, useDMS);
  Settings::Set(SETTINGS_KEY_USE_DMS, !useDMS);
  [self updateCoordinates];
  [self layoutSubviews];
}

- (void)coordinatesPress:(id)sender
{
  [self.delegate infoCellDidPressCoordinates:self withGestureRecognizer:sender];
}

- (void)selectedColorViewDidPress:(SelectedColorView *)selectedColorView
{
  [self.delegate infoCellDidPressColorSelector:self];
}

- (UIImageView *)separatorView
{
  if (!_separatorView)
  {
    _separatorView = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, self.width, 1)];
    _separatorView.image = [[UIImage imageNamed:@"PlacePageSeparator"] resizableImageWithCapInsets:UIEdgeInsetsZero];
    _separatorView.autoresizingMask = UIViewAutoresizingFlexibleWidth;
  }
  return _separatorView;
}

- (UILabel *)distanceLabel
{
  if (!_distanceLabel)
  {
    _distanceLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _distanceLabel.backgroundColor = [UIColor clearColor];
    _distanceLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5];
    _distanceLabel.textAlignment = NSTextAlignmentLeft;
    _distanceLabel.textColor = [UIColor blackColor];
    _distanceLabel.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _distanceLabel;
}

- (CopyView *)coordinatesView
{
  if (!_coordinatesView)
  {
    _coordinatesView = [[CopyView alloc] initWithFrame:CGRectZero];
    _coordinatesView.backgroundColor = [UIColor clearColor];
    _coordinatesView.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
    UILongPressGestureRecognizer * press = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(coordinatesPress:)];
    [_coordinatesView addGestureRecognizer:press];
    UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(coordinatesTap:)];
    [_coordinatesView addGestureRecognizer:tap];
  }
  return _coordinatesView;
}

- (UILabel *)latitudeLabel
{
  if (!_latitudeLabel)
  {
    _latitudeLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _latitudeLabel.backgroundColor = [UIColor clearColor];
    _latitudeLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:18];
    _latitudeLabel.textAlignment = NSTextAlignmentLeft;
    _latitudeLabel.textColor = [UIColor blackColor];
  }
  return _latitudeLabel;
}

- (UILabel *)longitudeLabel
{
  if (!_longitudeLabel)
  {
    _longitudeLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _longitudeLabel.backgroundColor = [UIColor clearColor];
    _longitudeLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:18];
    _longitudeLabel.textAlignment = NSTextAlignmentLeft;
    _longitudeLabel.textColor = [UIColor blackColor];
  }
  return _longitudeLabel;
}

- (SelectedColorView *)selectedColorView
{
  if (!_selectedColorView)
  {
    _selectedColorView = [[SelectedColorView alloc] initWithFrame:CGRectMake(0, 0, 44, 44)];
    _selectedColorView.delegate = self;
    _selectedColorView.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _selectedColorView;
}

- (SmallCompassView *)compassView
{
  if (!_compassView)
    _compassView = [[SmallCompassView alloc] init];
  return _compassView;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
