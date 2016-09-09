#include "platform/measurement_utils.hpp"

@interface MWMSettings : NSObject

+ (BOOL)adServerForbidden;
+ (void)setAdServerForbidden:(BOOL)adServerForbidden;

+ (BOOL)adForbidden;

+ (BOOL)autoDownloadEnabled;
+ (void)setAutoDownloadEnabled:(BOOL)autoDownloadEnabled;

+ (measurement_utils::Units)measurementUnits;
+ (void)setMeasurementUnits:(measurement_utils::Units)measurementUnits;

+ (BOOL)zoomButtonsEnabled;
+ (void)setZoomButtonsEnabled:(BOOL)zoomButtonsEnabled;

+ (BOOL)compassCalibrationEnabled;
+ (void)setCompassCalibrationEnabled:(BOOL)compassCalibrationEnabled;

+ (BOOL)statisticsEnabled;

+ (BOOL)autoNightModeEnabled;
+ (void)setAutoNightModeEnabled:(BOOL)autoNightModeEnabled;

+ (BOOL)routingDisclaimerApproved;
+ (void)setRoutingDisclaimerApproved;

+ (NSString *)spotlightLocaleLanguageId;
+ (void)setSpotlightLocaleLanguageId:(NSString *)spotlightLocaleLanguageId;

@end
