#import "SwiftBridge.h"

#include "platform/measurement_utils.hpp"

@interface MWMSettings : NSObject

+ (BOOL)adServerForbidden;
+ (void)setAdServerForbidden:(BOOL)adServerForbidden;

+ (BOOL)adForbidden;
+ (void)setAdForbidden:(BOOL)adForbidden;

+ (BOOL)autoDownloadEnabled;
+ (void)setAutoDownloadEnabled:(BOOL)autoDownloadEnabled;

+ (measurement_utils::Units)measurementUnits;
+ (void)setMeasurementUnits:(measurement_utils::Units)measurementUnits;

+ (BOOL)zoomButtonsEnabled;
+ (void)setZoomButtonsEnabled:(BOOL)zoomButtonsEnabled;

+ (BOOL)compassCalibrationEnabled;
+ (void)setCompassCalibrationEnabled:(BOOL)compassCalibrationEnabled;

+ (BOOL)statisticsEnabled;
+ (void)setStatisticsEnabled:(BOOL)statisticsEnabled;

+ (MWMTheme)theme;
+ (void)setTheme:(MWMTheme)theme;

+ (BOOL)routingDisclaimerApproved;
+ (void)setRoutingDisclaimerApproved;

+ (NSString *)spotlightLocaleLanguageId;
+ (void)setSpotlightLocaleLanguageId:(NSString *)spotlightLocaleLanguageId;

+ (BOOL)largeFontSize;
+ (void)setLargeFontSize:(BOOL)largeFontSize;

@end
