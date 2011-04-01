//
//  IPhoneLocator.mm
//  Maps
//
//  Created by Siarhei Rachytski on 3/28/11.
//  Copyright 2011 MapsWithMe. All rights reserved.
//

#include "IPhoneLocator.h"
#import <Foundation/NSDate.h>

@implementation LocatorThunk

@synthesize locator;

- (void) initWithLocator : (iphone::Locator *) loc
{
  self.locator = loc;
}

- (void) OnLocation: (m2::PointD const &) mercatorPoint
    withErrorRadius: (double) errorRadius
			withTimestamp: (NSDate *) timestamp;
{
  self.locator->locationUpdate(mercatorPoint,
                               errorRadius,
                               [timestamp timeIntervalSince1970],
                               [[NSDate date] timeIntervalSince1970]);
}

- (void) OnHeading: (CLHeading*) heading
{
  self.locator->headingUpdate(heading.trueHeading,
                              heading.magneticHeading,
                              heading.headingAccuracy);
}

- (void) OnLocationError: (NSString*) error
{
  NSLog(@"Error : %@", error);
}

@end

namespace iphone
{
  Locator::Locator()
  {
    m_thunk = [LocatorThunk alloc];
    [m_thunk initWithLocator:this];
    m_locationController = [[UserLocationController alloc] initWithDelegate:m_thunk];
  }

  Locator::~Locator()
  {
    stop();
    [m_locationController release];
    [m_thunk release];
  }

  void Locator::start(EMode mode)
  {
    ::Locator::setMode(mode);

    if (mode == ERoughMode)
      [m_locationController.locationManager startMonitoringSignificantLocationChanges];
    else
      [m_locationController.locationManager startUpdatingLocation];

    if ([CLLocationManager headingAvailable])
    {
      m_locationController.locationManager.headingFilter = 1;
      [m_locationController.locationManager startUpdatingHeading];
    }
  }

  void Locator::stop()
  {
    if (mode() == ERoughMode)
      [m_locationController.locationManager stopMonitoringSignificantLocationChanges];
    else
      [m_locationController.locationManager stopUpdatingLocation];

    if ([CLLocationManager headingAvailable])
      [m_locationController.locationManager stopUpdatingHeading];
  }

  void Locator::setMode(EMode mode)
  {
    EMode oldMode = ::Locator::mode();
    ::Locator::setMode(mode);
    callOnChangeModeFns(oldMode, mode);

    if (mode == ERoughMode)
    {
      [m_locationController.locationManager stopUpdatingLocation];
      [m_locationController.locationManager startMonitoringSignificantLocationChanges];
    }
    else
    {
      [m_locationController.locationManager stopMonitoringSignificantLocationChanges];
      [m_locationController.locationManager startUpdatingLocation];
    }
  }

  void Locator::locationUpdate(m2::PointD const & pt, double errorRadius, double locTimeStamp, double curTimeStamp)
  {
    callOnUpdateLocationFns(pt, errorRadius, locTimeStamp, curTimeStamp);
  }

  void Locator::headingUpdate(double trueHeading, double magneticHeading, double accuracy)
  {
    callOnUpdateHeadingFns(trueHeading, magneticHeading, accuracy);
  }
}

