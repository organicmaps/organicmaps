//
//  PlaceAndCompasView.m
//  Maps
//
//  Created by Kirill on 22/06/2013.
//  Copyright (c) 2013 MapsWithMe. All rights reserved.
//

#import "PlaceAndCompasView.h"
#import "MapsAppDelegate.h"
#import "CompassView.h"
#import "CompassView.h"
#import "Framework.h"

#include "../../../map/measurement_utils.hpp"
#include "../../../geometry/distance_on_sphere.hpp"

#define MARGIN 20
#define SMALLMARGIN 5
#define COMPASSSIDE 150
#define DISTANCEHEIGHT 20
#define NAMEFONTSIZE 25
#define SECONDNAMEFONTSIZE 20
#define CIRCLEDIAMETER 190

@interface CircleView : UIView
@end

@implementation CircleView

-(id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  if (self)
    self.opaque = NO;
  return self;
}

- (void)drawRect:(CGRect)rect
{
  CGContextRef ctx = UIGraphicsGetCurrentContext();
  CGContextAddEllipseInRect(ctx, rect);
  CGContextSetFillColor(ctx, CGColorGetComponents([[UIColor colorWithRed:255.0 green:255.0 blue:255.0 alpha:0.5] CGColor]));
  CGContextFillPath(ctx);
}

@end

@interface PlaceAndCompasView()
{
  double m_xGlobal, m_yGlobal;
  LocationManager * m_locationManager;
  double m_currentHeight;
}

@property (nonatomic, copy) NSString * name;
@property (nonatomic, copy) NSString * secondaryInfo;
@property (nonatomic, retain) CompassView * compass;
@property (nonatomic, retain) UITextView * nameView;
@property (nonatomic, retain) UITextView * secondaryInfoView;
@property (nonatomic, retain) UILabel * distanceLabel;
@property (nonatomic, retain) CircleView * circle;
@property (nonatomic, assign) double screenWidth;


@end

@implementation PlaceAndCompasView

- (id)initWithName:(NSString *)placeName placeSecondaryName:(NSString *)placeSecondaryName placeGlobalPoint:(CGPoint)point width:(CGFloat)width
{
  self = [super initWithFrame:CGRectZero];
  if (self)
  {
    m_xGlobal = point.x;
    m_yGlobal = point.y;

    _circle = [[CircleView alloc] init];

    _distanceLabel = [[UILabel alloc] init];
    self.distanceLabel.backgroundColor = [UIColor clearColor];

    _compass = [[CompassView alloc] initWithFrame:CGRectZero];
    self.compass.color = [UIColor blackColor];

    m_locationManager = [MapsAppDelegate theApp].m_locationManager;
    [m_locationManager start:self];

    if ([placeName length] == 0)
    {
      _name = [[NSString alloc] initWithString:placeSecondaryName];
      _secondaryInfo = [[NSString alloc] initWithString:@""];
    }
    else
    {
      _name = [[NSString alloc] initWithString:placeName];
      _secondaryInfo = [[NSString alloc] initWithString:placeSecondaryName];
    }
    _screenWidth = width;

    _nameView = [[UITextView alloc] initWithFrame:CGRectZero];
    self.nameView.text = self.name;
    self.nameView.editable = NO;
    self.nameView.backgroundColor = [UIColor clearColor];
    self.nameView.font = [UIFont fontWithName:@"Helvetica" size:NAMEFONTSIZE];
    self.nameView.scrollEnabled = NO;

    _secondaryInfoView = [[UITextView alloc] initWithFrame:CGRectZero];

    self.secondaryInfoView.text = self.secondaryInfo;
    self.secondaryInfoView.editable = NO;
    self.secondaryInfoView.backgroundColor = [UIColor clearColor];
    self.secondaryInfoView.font = [UIFont fontWithName:@"Helvetica" size:SECONDNAMEFONTSIZE];
    self.secondaryInfoView.scrollEnabled = NO;

    [self addSubview:self.circle];
    [self addSubview:self.nameView];
    [self addSubview:self.secondaryInfoView];
    [self addSubview:self.compass];
    [self addSubview:self.distanceLabel];

    self.compass.angle = [self getAzimut];

    [self drawView];
  }
  return self;
}

-(void)dealloc
{
  [m_locationManager stop:self];

  self.name = nil;
  self.secondaryInfo = nil;
  self.compass = nil;
  self.distanceLabel = nil;

  self.nameView = nil;
  self.secondaryInfoView = nil;
  [super dealloc];
}

-(void)drawView
{
  if ([self canShowArrow])
    [self drawWithArrow];
  else
    [self drawWithoutArrow];
}

-(void)drawWithoutArrow
{
  [self.compass setShowArrow:NO];
  self.distanceLabel.frame = CGRectZero;
  self.circle.frame = CGRectZero;
  double width = [self getCurrentSuperViewWidth];

  if ([self.name length] == 0)
  {
    self.nameView.frame = CGRectZero;
    [self.secondaryInfoView setFont:[UIFont fontWithName:@"Helvetica" size:NAMEFONTSIZE]];
  }
  else
  {
    self.nameView.frame = CGRectMake(2 * SMALLMARGIN, 2 * SMALLMARGIN, width - MARGIN, [self getTextHeight:self.name font:[UIFont fontWithName:@"Helvetica" size:NAMEFONTSIZE]]);
  }

  if ([self.secondaryInfo length] == 0)
    self.secondaryInfoView.frame = CGRectZero;
  else
    self.secondaryInfoView.frame = CGRectMake(2 * SMALLMARGIN, SMALLMARGIN + _nameView.frame.size.height
                                              , width - MARGIN, [self getTextHeight:self.secondaryInfo font:[UIFont fontWithName:@"Helvetica" size:SECONDNAMEFONTSIZE]]);
  self.frame = CGRectMake(0, 0, width, [self countHeight]);
}

-(void)drawWithArrow
{
  double width = [self getCurrentSuperViewWidth];
  [self.compass setShowArrow:YES];
  self.circle.frame = CGRectMake(width/2 - CIRCLEDIAMETER/2 - SMALLMARGIN, 2 * SMALLMARGIN, CIRCLEDIAMETER, CIRCLEDIAMETER);
  self.compass.frame = CGRectMake(width/2 - COMPASSSIDE/2 - SMALLMARGIN, 2 * SMALLMARGIN + (CIRCLEDIAMETER - COMPASSSIDE)/2 , COMPASSSIDE, COMPASSSIDE);

  self.distanceLabel.frame = CGRectMake(0, COMPASSSIDE + 5 * SMALLMARGIN, width, 20);
  [self.distanceLabel setTextAlignment:NSTextAlignmentCenter];

  if ([self.name length] == 0)
  {
    self.nameView.frame = CGRectZero;
    self.secondaryInfoView.frame = CGRectZero;
    self.frame = CGRectMake(0, 0, width - MARGIN, CIRCLEDIAMETER + 2 * SMALLMARGIN);
    return;
  }
  else
  {
    self.nameView.frame = CGRectMake(2 * SMALLMARGIN, 4 * SMALLMARGIN + CIRCLEDIAMETER, width - MARGIN, [self getTextHeight:self.name font:[UIFont fontWithName:@"Helvetica" size:NAMEFONTSIZE]]);
    [self.secondaryInfoView setFont:[UIFont fontWithName:@"Helvetica" size:SECONDNAMEFONTSIZE]];
  }

  if ([self.secondaryInfo length] == 0)
    self.secondaryInfoView.frame = CGRectZero;
  else
    self.secondaryInfoView.frame = CGRectMake(2 * SMALLMARGIN, self.nameView.frame.origin.y + self.nameView.frame.size.height
                                        , width - MARGIN, [self getTextHeight:self.secondaryInfo font:[UIFont fontWithName:@"Helvetica" size:SECONDNAMEFONTSIZE]]);

  self.frame = CGRectMake(0, 0, width, [self countHeight]);
}

#pragma mark - Location Observer Delegate

- (void)onLocationError:(location::TLocationError)errorCode
{
  // Don't know what to do with this method?
}

- (void)onLocationUpdate:(location::GpsInfo const &)info
{
  double const d = ms::DistanceOnEarth(info.m_latitude, info.m_longitude,
                                       MercatorBounds::YToLat(m_yGlobal),
                                       MercatorBounds::XToLon(m_xGlobal));
  string distance = "";
  (void) MeasurementUtils::FormatDistance(d, distance);
  self.distanceLabel.text = [NSString stringWithUTF8String:distance.c_str()];
}

- (void)onCompassUpdate:(location::CompassInfo const &)info
{
  double lat, lon;
  if (![m_locationManager getLat:lat Lon:lon])
    return;
  double const northRad = (info.m_trueHeading < 0) ? info.m_magneticHeading : info.m_trueHeading;
  self.compass.angle = ang::AngleTo(m2::PointD(MercatorBounds::LonToX(lon),
                                          MercatorBounds::LatToY(lat)), m2::PointD(m_xGlobal, m_yGlobal)) + northRad;
}

-(BOOL)canShowArrow
{
  if  ([self getAzimut] >= 0.0)
    return YES;
  else
    return NO;
}

-(double)getAzimut
{
  double azimut = -1.0;
  double lat = 0.0, lon = 0.0;

  if ([m_locationManager getLat:lat Lon:lon])
  {
    double north = -1.0;
    [m_locationManager getNorthRad:north];
    if (north >= 0.0)
    {
      azimut = ang::AngleTo(m2::PointD(MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat)),
                            m2::PointD(m_xGlobal, m_yGlobal)) + north;
      azimut = ang::AngleIn2PI(azimut);
    }
  }
  return azimut;
}

-(double)getTextHeight:(NSString *)text font:(UIFont *)font
{
  return  [text sizeWithFont:font constrainedToSize:CGSizeMake([self getCurrentSuperViewWidth] - 2 * MARGIN, CGFLOAT_MAX) lineBreakMode:NSLineBreakByCharWrapping].height + 2 * SMALLMARGIN;
}

-(CGFloat) countHeight
{
  if ([self.secondaryInfo length])
    return self.secondaryInfoView.frame.origin.y + self.secondaryInfoView.frame.size.height + SMALLMARGIN;
  else
    return self.nameView.frame.origin.y + self.nameView.frame.size.height + SMALLMARGIN;
}

-(CGFloat)getCurrentSuperViewWidth
{
  double width = self.superview.bounds.size.width;
  if (width == 0)
    width = _screenWidth;
  return width;
}

@end
