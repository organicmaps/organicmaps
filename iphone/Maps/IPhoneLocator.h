//
//  IPhoneLocator.h
//  Maps
//
//  Created by Siarhei Rachytski on 3/28/11.
//  Copyright 2011 MapsWithMe. All rights reserved.
//

#include "../../map/locator.hpp"
#include "UserLocationController.h"

namespace iphone
{
  class Locator;
}

@interface LocatorThunk : NSObject<UserLocationControllerDelegate>
{
  iphone::Locator * m_locator;
}

@property (nonatomic, assign) iphone::Locator * locator;

- (void) initWithLocator : (iphone::Locator*) locator;

- (void) OnLocation: (m2::PointD const &) mercatorPoint
    withErrorRadius: (double) errorRadius
			withTimestamp: (NSDate *) timestamp;
- (void) OnHeading: (CLHeading *)heading;
- (void) OnLocationError: (NSString *) errorDescription;

@end

namespace iphone
{
  class Locator : public ::Locator
  {
  private:

    LocatorThunk * m_thunk;
    UserLocationController * m_locationController;

  public:

    Locator();
    ~Locator();

    void start(EMode mode);
    void stop();
    void setMode(EMode mode);

    void locationUpdate(m2::PointD const & pt, double errorRadius, double locTimeStamp, double curTimeStamp);
    void headingUpdate(double trueHeading, double magneticHeading, double accuracy);
  };
}


