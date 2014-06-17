
#import "PlacePageInfoCell.h"
#import "UIKitCategories.h"
#import "LocationManager.h"
#import "MapsAppDelegate.h"
#import "Framework.h"
#include "../../../platform/settings.hpp"
#include "../../../map/measurement_utils.hpp"
#include "../../../geometry/distance_on_sphere.hpp"
#import "ContextViews.h"

#define SETTINGS_KEY_USE_DMS "UseDMSCoordinates"

@interface PlacePageInfoCell () <SelectedColorViewDelegate>

@property (nonatomic) UILabel * distanceLabel;
@property (nonatomic) CopyLabel * addressLabel;
@property (nonatomic) CopyLabel * coordinatesLabel;
@property (nonatomic) SmallCompassView * compassView;
@property (nonatomic) UIImageView * separatorView;
@property (nonatomic) SelectedColorView * selectedColorView;

@property (nonatomic) m2::PointD pinPoint;

@end

@implementation PlacePageInfoCell

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  self.selectionStyle = UITableViewCellSelectionStyleNone;

  [self addSubview:self.compassView];
  [self addSubview:self.distanceLabel];
  [self addSubview:self.addressLabel];
  [self addSubview:self.coordinatesLabel];
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

  string const coords = useDMS ? MeasurementUtils::FormatMercatorAsDMS(self.pinPoint)
                               : MeasurementUtils::FormatMercator(self.pinPoint);
  self.coordinatesLabel.text = [NSString stringWithUTF8String:coords.c_str()];
}

- (void)setAddress:(NSString *)address pinPoint:(m2::PointD)point
{
  self.pinPoint = point;
  self.addressLabel.text = address;
  [self updateCoordinates];
  self.distanceLabel.text = [self distance];
}

- (void)setColor:(UIColor *)color
{
  [self.selectedColorView setColor:color];
}

#define ADDRESS_LEFT_SHIFT 19
#define RIGHT_SHIFT 55
#define DISTANCE_LEFT_SHIFT 55

#define ADDRESS_FONT [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5]

- (void)layoutSubviews
{
  CGFloat addressY;
  if ([[MapsAppDelegate theApp].m_locationManager enabledOnMap])
  {
    self.compassView.origin = CGPointMake(19, 17);
    self.compassView.hidden = NO;
    self.distanceLabel.frame = CGRectMake(DISTANCE_LEFT_SHIFT, 18, self.width - DISTANCE_LEFT_SHIFT - RIGHT_SHIFT, 24);
    self.distanceLabel.hidden = NO;
    addressY = 55;
  }
  else
  {
    self.compassView.hidden = YES;
    self.distanceLabel.hidden = YES;
    addressY = 15;
  }
  self.addressLabel.width = self.width - ADDRESS_LEFT_SHIFT - RIGHT_SHIFT;
  [self.addressLabel sizeToFit];
  self.addressLabel.origin = CGPointMake(ADDRESS_LEFT_SHIFT, addressY);
  // coordinates label is wider than address label, to fit long DMS coordinates
  self.coordinatesLabel.frame = CGRectMake(ADDRESS_LEFT_SHIFT, self.addressLabel.maxY + 10, self.width - ADDRESS_LEFT_SHIFT - ADDRESS_LEFT_SHIFT, 24);

  self.selectedColorView.center = CGPointMake(self.width - 32, 27);

  self.separatorView.maxY = self.height;
  CGFloat const shift = 15;
  self.separatorView.width = self.width - 2 * shift;
  self.separatorView.minX = shift;

  self.backgroundColor = [UIColor clearColor];
}

+ (CGFloat)cellHeightWithAddress:(NSString *)address viewWidth:(CGFloat)viewWidth
{
  CGFloat addressHeight = [address sizeWithDrawSize:CGSizeMake(viewWidth - ADDRESS_LEFT_SHIFT - RIGHT_SHIFT, 200) font:ADDRESS_FONT].height;
  return addressHeight + ([[MapsAppDelegate theApp].m_locationManager enabledOnMap] ? 110 : 66);
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
    _distanceLabel.textColor = [UIColor whiteColor];
    _distanceLabel.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
  }
  return _distanceLabel;
}

- (CopyLabel *)addressLabel
{
  if (!_addressLabel)
  {
    _addressLabel = [[CopyLabel alloc] initWithFrame:CGRectZero];
    _addressLabel.backgroundColor = [UIColor clearColor];
    _addressLabel.font = ADDRESS_FONT;
    _addressLabel.numberOfLines = 0;
    _addressLabel.lineBreakMode = NSLineBreakByWordWrapping;
    _addressLabel.textAlignment = NSTextAlignmentLeft;
    _addressLabel.textColor = [UIColor whiteColor];
    _addressLabel.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
    UILongPressGestureRecognizer * press = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(addressPress:)];
    [_addressLabel addGestureRecognizer:press];
  }
  return _addressLabel;
}

- (CopyLabel *)coordinatesLabel
{
  if (!_coordinatesLabel)
  {
    _coordinatesLabel = [[CopyLabel alloc] initWithFrame:CGRectZero];
    _coordinatesLabel.backgroundColor = [UIColor clearColor];
    _coordinatesLabel.font = [UIFont fontWithName:@"HelveticaNeue-Light" size:17.5];
    _coordinatesLabel.textAlignment = NSTextAlignmentLeft;
    _coordinatesLabel.textColor = [UIColor whiteColor];
    _coordinatesLabel.autoresizingMask = UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleBottomMargin;
    UILongPressGestureRecognizer * press = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(coordinatesPress:)];
    [_coordinatesLabel addGestureRecognizer:press];
    UITapGestureRecognizer * tap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(coordinatesTap:)];
    [_coordinatesLabel addGestureRecognizer:tap];
  }
  return _coordinatesLabel;
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
