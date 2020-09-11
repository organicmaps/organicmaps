//
//  MPGeolocationProvider.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPGeolocationProvider.h"

#import "MPCoreInstanceProvider.h"
#import "MPLogging.h"
#import "MPTimer.h"
#import "MPConsentManager.h"
#import "MPConsentChangedNotification.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

// The minimum distance (meters) a device must move horizontally before CLLocationManager generates
// an update event. Used to limit the amount of events generated.
const CLLocationDistance kMPCityBlockDistanceFilter = 100.0;

// The duration (seconds) for which we want to listen for location updates (i.e. how long we wait to
// call -stopUpdatingLocation after calling -startUpdatingLocation).
const NSTimeInterval kMPLocationUpdateDuration = 15.0;

// The duration (seconds) between calls to -startUpdatingLocation.
const NSTimeInterval kMPLocationUpdateInterval = 10.0 * 60.0;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPGeolocationProvider () <CLLocationManagerDelegate>

@property (nonatomic, readwrite) CLLocation *lastKnownLocation;
@property (nonatomic) CLLocationManager *locationManager;
@property (nonatomic) BOOL authorizedForLocationServices;
@property (nonatomic) NSDate *timeOfLastLocationUpdate;
@property (nonatomic) MPTimer *nextLocationUpdateTimer;
@property (nonatomic) MPTimer *locationUpdateDurationTimer;
// Raw locationUpdatesEnabled value set by publisher
@property (nonatomic) BOOL rawLocationUpdatesEnabled;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPGeolocationProvider

+ (instancetype)sharedProvider
{
    static MPGeolocationProvider *sharedProvider = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedProvider = [[[self class] alloc] init];
    });
    return sharedProvider;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _rawLocationUpdatesEnabled = YES;

        _locationManager = CLLocationManager.new;
        _locationManager.delegate = self;
        _locationManager.distanceFilter = kMPCityBlockDistanceFilter;

        // CLLocationManager's `location` property may already contain location data upon
        // initialization (for example, if the application uses significant location updates).
        CLLocation *existingLocation = _locationManager.location;
        if ([self locationHasValidCoordinates:existingLocation]) {
            _lastKnownLocation = existingLocation;
            MPLogDebug(@"Found previous location information.");
        }

        // Avoid processing location updates when the application enters the background.
        [[NSNotificationCenter defaultCenter] addObserverForName:UIApplicationDidEnterBackgroundNotification object:[UIApplication sharedApplication] queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *note) {
            [self stopAllCurrentOrScheduledLocationUpdates];
        }];

        // Re-activate location updates when the application comes back to the foreground.
        [[NSNotificationCenter defaultCenter] addObserverForName:UIApplicationWillEnterForegroundNotification object:[UIApplication sharedApplication] queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *note) {
            if (self.locationUpdatesEnabled) {
                [self resumeLocationUpdatesAfterBackgrounding];
            }
        }];

        if ([MPConsentManager sharedManager].canCollectPersonalInfo) {
            [self startRecurringLocationUpdates];
        }

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(consentStateChanged:) name:kMPConsentChangedNotification object:nil];

    }
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIApplicationDidEnterBackgroundNotification object:[UIApplication sharedApplication]];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIApplicationWillEnterForegroundNotification object:[UIApplication sharedApplication]];
}

#pragma mark - Public

- (CLLocation *)lastKnownLocation
{
    if (!self.locationUpdatesEnabled) {
        return nil;
    }

    return _lastKnownLocation;
}

- (BOOL)locationUpdatesEnabled
{
    return self.rawLocationUpdatesEnabled && [MPConsentManager sharedManager].canCollectPersonalInfo;
}

- (void)setLocationUpdatesEnabled:(BOOL)enabled
{
    self.rawLocationUpdatesEnabled = enabled;
    [self startOrStopLocationUpdates];
}

- (void)startOrStopLocationUpdates
{
    if (!self.locationUpdatesEnabled) {
        [self stopAllCurrentOrScheduledLocationUpdates];
        self.lastKnownLocation = nil;
    } else if (![self.locationUpdateDurationTimer isValid] && ![self.nextLocationUpdateTimer isValid]) {
        [self startRecurringLocationUpdates];
    }
}

#pragma mark - Internal

- (void)setAuthorizedForLocationServices:(BOOL)authorizedForLocationServices
{
    _authorizedForLocationServices = authorizedForLocationServices;

    if (_authorizedForLocationServices && [CLLocationManager locationServicesEnabled]) {
        [self startRecurringLocationUpdates];
    } else {
        [self stopAllCurrentOrScheduledLocationUpdates];
        self.lastKnownLocation = nil;
    }
}

- (BOOL)isAuthorizedStatus:(CLAuthorizationStatus)status
{
    return (status == kCLAuthorizationStatusAuthorizedAlways) || (status == kCLAuthorizationStatusAuthorizedWhenInUse);
}

/**
 * Tells the location provider to start periodically retrieving new location data.
 *
 * The location provider will activate its underlying location manager for a specified amount of
 * time, during which the provider may receive delegate callbacks about location updates. After this
 * duration, the provider will schedule a future update. These updates can be stopped via
 * -stopAllCurrentOrScheduledLocationUpdates.
 */
- (void)startRecurringLocationUpdates
{
    self.timeOfLastLocationUpdate = [NSDate date];

    if (![CLLocationManager locationServicesEnabled] || ![self isAuthorizedStatus:[CLLocationManager authorizationStatus]]) {
        MPLogDebug(@"Will not start location updates: the application is not authorized "
                   @"for location services.");
        return;
    }

    if (!self.locationUpdatesEnabled) {
        MPLogDebug(@"Will not start location updates because they have been disabled.");
        return;
    }

    [self.locationManager startUpdatingLocation];

    [self.locationUpdateDurationTimer invalidate];
    self.locationUpdateDurationTimer = [MPTimer timerWithTimeInterval:kMPLocationUpdateDuration
                                                               target:self
                                                             selector:@selector(currentLocationUpdateDidFinish)
                                                              repeats:NO];
    [self.locationUpdateDurationTimer scheduleNow];
}

- (void)currentLocationUpdateDidFinish
{
    MPLogDebug(@"Stopping the current location update session and scheduling the next session.");
    [self.locationUpdateDurationTimer invalidate];
    [self.locationManager stopUpdatingLocation];

    [self scheduleNextLocationUpdateAfterDelay:kMPLocationUpdateInterval];
}

- (void)scheduleNextLocationUpdateAfterDelay:(NSTimeInterval)delay
{
    MPLogDebug(@"Next user location update due in %.1f seconds.", delay);
    [self.nextLocationUpdateTimer invalidate];
    self.nextLocationUpdateTimer = [MPTimer timerWithTimeInterval:delay
                                                           target:self
                                                         selector:@selector(startRecurringLocationUpdates)
                                                          repeats:NO];
    [self.nextLocationUpdateTimer scheduleNow];
}

- (void)stopAllCurrentOrScheduledLocationUpdates
{
    MPLogDebug(@"Stopping any scheduled location updates.");
    [self.locationUpdateDurationTimer invalidate];
    [self.locationManager stopUpdatingLocation];

    [self.nextLocationUpdateTimer invalidate];
}

- (void)resumeLocationUpdatesAfterBackgrounding
{
    NSTimeInterval timeSinceLastUpdate = [[NSDate date] timeIntervalSinceDate:self.timeOfLastLocationUpdate];

    if (timeSinceLastUpdate >= kMPLocationUpdateInterval) {
        MPLogDebug(@"Last known user location is stale. Updating location.");
        [self startRecurringLocationUpdates];
    } else if (timeSinceLastUpdate >= 0) {
        NSTimeInterval timeToNextUpdate = kMPLocationUpdateInterval - timeSinceLastUpdate;
        [self scheduleNextLocationUpdateAfterDelay:timeToNextUpdate];
    } else {
        [self scheduleNextLocationUpdateAfterDelay:kMPLocationUpdateInterval];
    }
}

#pragma mark - CLLocation Helpers

- (BOOL)isLocation:(CLLocation *)location betterThanLocation:(CLLocation *)otherLocation
{
    if (!otherLocation) {
        return YES;
    }

    // Nil locations and locations with invalid horizontal accuracy are worse than any location.
    if (![self locationHasValidCoordinates:location]) {
        return NO;
    }

    if ([self isLocation:location olderThanLocation:otherLocation]) {
        return NO;
    }

    return YES;
}

- (BOOL)locationHasValidCoordinates:(CLLocation *)location
{
    return location && location.horizontalAccuracy > 0;
}

- (BOOL)isLocation:(CLLocation *)location olderThanLocation:(CLLocation *)otherLocation
{
    return [location.timestamp timeIntervalSinceDate:otherLocation.timestamp] < 0;
}

#pragma mark - <CLLocationManagerDelegate> (iOS 6.0+)

- (void)locationManager:(CLLocationManager *)manager didChangeAuthorizationStatus:(CLAuthorizationStatus)status
{
    MPLogDebug(@"Location authorization status changed to: %ld", (long)status);

    switch (status) {
        case kCLAuthorizationStatusNotDetermined:
        case kCLAuthorizationStatusDenied:
        case kCLAuthorizationStatusRestricted:
            self.authorizedForLocationServices = NO;
            break;
        case kCLAuthorizationStatusAuthorizedAlways:
        case kCLAuthorizationStatusAuthorizedWhenInUse:
            self.authorizedForLocationServices = YES;
            break;
        default:
            self.authorizedForLocationServices = NO;
            break;
    }
}

- (void)locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    for (CLLocation *location in locations) {
        if ([self isLocation:location betterThanLocation:self.lastKnownLocation]) {
            self.lastKnownLocation = location;
            MPLogDebug(@"Updated last known user location: %@", location);
        }
    }
}

- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error
{
    if (error.code == kCLErrorDenied) {
        MPLogDebug(@"Location manager failed: the user has denied access to location services.");
        [self stopAllCurrentOrScheduledLocationUpdates];
    } else if (error.code == kCLErrorLocationUnknown) {
        MPLogDebug(@"Location manager could not obtain a location right now.");
    }
}

#pragma mark - Consent

- (void)consentStateChanged:(NSNotification *)notification
{
    [self startOrStopLocationUpdates];
}

@end
