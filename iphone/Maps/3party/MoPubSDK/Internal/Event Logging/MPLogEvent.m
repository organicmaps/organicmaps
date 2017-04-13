//
//  MPLogEvent.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPLogEvent.h"
#import "MPIdentityProvider.h"
#import "MPInternalUtils.h"
#import "MPCoreInstanceProvider.h"
#import "MPConstants.h"
#import "MPReachability.h"
#import "MPGeolocationProvider.h"

NSUInteger const IOS_APP_PLATFORM_ID = 1;
NSUInteger const IOS_SDK_PRODUCT_ID = 0;
NSString * const IOS_MANUFACTURER_NAME = @"Apple";

/*
 * Event names.
 */
NSString * const MPLogEventNameAdRequest = @"ad_request";
NSString * const MPLogEventNameClickthroughDwellTime = @"clickthrough_dwell_time";

/*
 * Event categories.
 */
NSString * const MPLogEventCategoryRequests = @"requests";
NSString * const MPLogEventCategoryNativeVideo = @"native_video";
NSString * const MPLogEventCategoryAdInteractions = @"ad_interactions";

@interface MPAdConfigurationLogEventProperties ()

@end

@implementation MPAdConfigurationLogEventProperties

- (instancetype)initWithConfiguration:(MPAdConfiguration *)configuration
{
    if (self = [super init]) {
        _adType = configuration.headerAdType;
        _adCreativeId = configuration.creativeId;
        _dspCreativeId = configuration.dspCreativeId;
        _adNetworkType = configuration.networkType;
        _adSize = configuration.preferredSize;

        NSDictionary *failURLQueryParameters = MPDictionaryFromQueryString([configuration.failoverURL query]);
        _requestId = failURLQueryParameters[@"request_id"];
        _adUnitId = failURLQueryParameters[@"id"];
    }
    return self;
}

@end


#pragma mark - Private properties

@interface MPLogEvent ()

@property (nonatomic, readwrite) NSDate *timestamp;
@property (nonatomic, readwrite) MPLogEventScribeCategory scribeCategory;
@property (nonatomic, copy, readwrite) NSString *sdkVersion;
@property (nonatomic, copy, readwrite) NSString *deviceModel;
@property (nonatomic, copy, readwrite) NSString *deviceOSVersion;
@property (nonatomic, readwrite) CGSize deviceSize;
@property (nonatomic, readwrite) double geoLat;
@property (nonatomic, readwrite) double geoLon;
@property (nonatomic, readwrite) double geoAccuracy;
@property (nonatomic, readwrite) MPLogEventNetworkType networkType;
@property (nonatomic, copy, readwrite) NSString *networkOperatorCode;
@property (nonatomic, copy, readwrite) NSString *networkOperatorName;
@property (nonatomic, copy, readwrite) NSString *networkISOCountryCode;
@property (nonatomic, readwrite) NSUInteger performanceDurationMs;
@property (nonatomic, copy, readwrite) NSString *networkSIMCode;
@property (nonatomic, copy, readwrite) NSString *networkSIMOperatorName;
@property (nonatomic, copy, readwrite) NSString *networkSimISOCountryCode;
@property (nonatomic, copy, readwrite) NSString *clientAdvertisingId;
@property (nonatomic, readwrite) BOOL clientDoNotTrack;

@end


@implementation MPLogEvent

- (instancetype)initWithEventCategory:(NSString *)eventCategory eventName:(NSString *)eventName
{
    if (self = [super init]) {

        MPCoreInstanceProvider *provider = [MPCoreInstanceProvider sharedProvider];

        [self setEventName:eventName];
        [self setEventCategory:eventCategory];

        [self setTimestamp:[NSDate date]];
        [self setScribeCategory:MPExchangeClientEventCategory];

        // SDK Info
        [self setSdkVersion:MP_SDK_VERSION];

        // Device info
        UIDevice *device = [UIDevice currentDevice];
        [self setDeviceModel:[device model]];
        [self setDeviceOSVersion:[device systemVersion]];

        [self setDeviceSize:MPScreenBounds().size];

        // Geolocation info
        CLLocation *location = [[[MPCoreInstanceProvider sharedProvider] sharedMPGeolocationProvider] lastKnownLocation];
        [self setGeoLat:location.coordinate.latitude];
        [self setGeoLon:location.coordinate.longitude];
        [self setGeoAccuracy:location.horizontalAccuracy];

        // Client info
        [self setClientDoNotTrack:![MPIdentityProvider advertisingTrackingEnabled]];
        // Note: we've chosen at this time to never send the real IDFA.
        [self setClientAdvertisingId:[MPIdentityProvider obfuscatedIdentifier]];


        // Network/Carrier info
        if ([provider sharedMPReachability].hasWifi) {
            [self setNetworkType:MPLogEventNetworkTypeWifi];
        } else if ([provider sharedMPReachability].hasCellular) {
            [self setNetworkType:MPLogEventNetworkTypeMobile];

            NSDictionary *carrierInfo = [[MPCoreInstanceProvider sharedProvider] sharedCarrierInfo];
            NSString *networkOperatorName = carrierInfo[@"carrierName"];
            NSString *mcc = carrierInfo[@"mobileCountryCode"];
            NSString *mnc = carrierInfo[@"mobileNetworkCode"];
            NSString *networkOperatorCode = [NSString stringWithFormat:@"%@%@", mcc, mnc];
            NSString *isoCountryCode = [[NSLocale currentLocale] objectForKey:NSLocaleCountryCode];

            [self setNetworkOperatorName:networkOperatorName];
            [self setNetworkSIMOperatorName:networkOperatorName];

            [self setNetworkSIMCode:networkOperatorCode];
            [self setNetworkOperatorCode:networkOperatorCode];

            [self setNetworkISOCountryCode:isoCountryCode];
            [self setNetworkSimISOCountryCode:isoCountryCode];

        } else {
            [self setNetworkType:MPLogEventNetworkTypeUnknown];
        }

    }

    return self;
}

- (void)setRequestURI:(NSString *)requestURI
{
    // We don't pass up the advertising identifier so we obfuscate it.
    _requestURI = [requestURI stringByReplacingOccurrencesOfString:[MPIdentityProvider identifier]
                                                        withString:[MPIdentityProvider obfuscatedIdentifier]];
}

- (NSUInteger)appPlatform
{
    return IOS_APP_PLATFORM_ID;
}

- (NSUInteger)sdkProduct
{
    return IOS_SDK_PRODUCT_ID;
}

- (NSString *)deviceManufacturer
{
    return IOS_MANUFACTURER_NAME;
}

/**
 * Current timestamp in ms since January 1, 1970 00:00:00.0 UTC (aka epoch time)
 */
- (NSUInteger)timestampAsEpoch
{
    return [[self timestamp] timeIntervalSince1970];
}

- (NSString *)eventCategoryAsString
{
    return @"exchange_client_event";
}

- (NSUInteger)networkTypeAsInteger
{
    switch ([self networkType]) {
        case MPLogEventNetworkTypeUnknown:
            return 0;
            break;
        case MPLogEventNetworkTypeEthernet:
            return 1;
            break;
        case MPLogEventNetworkTypeWifi:
            return 2;
            break;
        case MPLogEventNetworkTypeMobile:
            return 3;
            break;
        default:
            return 0;
            break;
    }
}

- (NSDictionary *)asDictionary
{
    NSMutableDictionary *d = [[NSMutableDictionary alloc] init];

    [d mp_safeSetObject:[self eventCategoryAsString] forKey:@"_category_"];
    [d mp_safeSetObject:[self eventName] forKey:@"name"];
    [d mp_safeSetObject:[self eventCategory] forKey:@"name_category"];

    [d mp_safeSetObject:@([self sdkProduct]) forKey:@"sdk_product"];
    [d mp_safeSetObject:[self sdkVersion] forKey:@"sdk_version"];

    [d mp_safeSetObject:[self adUnitId] forKey:@"ad_unit_id"];
    [d mp_safeSetObject:[self adCreativeId] forKey:@"ad_creative_id"];
    [d mp_safeSetObject:[self dspCreativeId] forKey:@"dsp_creative_id"];
    [d mp_safeSetObject:[self adType] forKey:@"ad_type"];
    [d mp_safeSetObject:[self adNetworkType] forKey:@"ad_network_type"];
    [d mp_safeSetObject:@(self.adSize.width) forKey:@"ad_width_px"];
    [d mp_safeSetObject:@(self.adSize.height) forKey:@"ad_height_px"];

    [d mp_safeSetObject:@([self appPlatform]) forKey:@"app_platform"];
    [d mp_safeSetObject:[self appName] forKey:@"app_name"];
    [d mp_safeSetObject:[self appStoreId] forKey:@"app_appstore_id"];
    [d mp_safeSetObject:[self appBundleId] forKey:@"app_bundle_id"];
    [d mp_safeSetObject:[self appVersion] forKey:@"app_version"];

    [d mp_safeSetObject:[self clientAdvertisingId] forKey:@"client_advertising_id"];
    [d mp_safeSetObject:@([self clientDoNotTrack]) forKey:@"client_do_not_track"];

    [d mp_safeSetObject:[self deviceManufacturer] forKey:@"device_manufacturer"];
    [d mp_safeSetObject:[self deviceModel] forKey:@"device_model"];
    [d mp_safeSetObject:[self deviceModel] forKey:@"device_product"];
    [d mp_safeSetObject:[self deviceOSVersion] forKey:@"device_os_version"];
    [d mp_safeSetObject:@(self.deviceSize.width) forKey:@"device_screen_width_px"];
    [d mp_safeSetObject:@(self.deviceSize.height) forKey:@"device_screen_height_px"];

    [d mp_safeSetObject:@([self geoLat]) forKey:@"geo_lat"];
    [d mp_safeSetObject:@([self geoLon]) forKey:@"geo_lon"];
    [d mp_safeSetObject:@([self geoAccuracy]) forKey:@"geo_accuracy_radius_meters"];

    [d mp_safeSetObject:@([self performanceDurationMs]) forKey:@"perf_duration_ms"];

    [d mp_safeSetObject:@([self networkType]) forKey:@"network_type"];
    [d mp_safeSetObject:[self networkOperatorCode] forKey:@"network_operator_code"];
    [d mp_safeSetObject:[self networkOperatorName] forKey:@"network_operator_name"];
    [d mp_safeSetObject:[self networkISOCountryCode] forKey:@"network_iso_country_code"];
    [d mp_safeSetObject:[self networkSimISOCountryCode] forKey:@"network_sim_iso_country_code"];

    [d mp_safeSetObject:[self requestId] forKey:@"req_id"];
    [d mp_safeSetObject:@([self requestStatusCode]) forKey:@"req_status_code"];
    [d mp_safeSetObject:[self requestURI] forKey:@"req_uri"];
    [d mp_safeSetObject:@([self requestRetries]) forKey:@"req_retries"];

    [d mp_safeSetObject:@([self timestampAsEpoch]) forKey:@"timestamp_client"];

    return d;
}

- (NSString *)serialize
{
    NSData *data = [NSJSONSerialization dataWithJSONObject:[self asDictionary] options:0 error:nil];
    NSString *jsonString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];

    return jsonString;
}

- (void)recordEndTime
{
    NSDate *currentTime = [NSDate date];
    NSTimeInterval durationMs = [currentTime timeIntervalSinceDate:self.timestamp] * 1000.0;

    self.performanceDurationMs = durationMs;
}

- (void)setLogEventProperties:(MPAdConfigurationLogEventProperties *)logEventProperties
{
    if (logEventProperties) {
        [self setAdType:logEventProperties.adType];
        [self setAdCreativeId:logEventProperties.adCreativeId];
        [self setDspCreativeId:logEventProperties.dspCreativeId];
        [self setAdNetworkType:logEventProperties.adNetworkType];
        [self setAdSize:logEventProperties.adSize];
        [self setRequestId:logEventProperties.requestId];
        [self setAdUnitId:logEventProperties.adUnitId];
    }
}

@end
