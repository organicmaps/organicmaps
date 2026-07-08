@protocol PromoManager <NSObject>

+ (BOOL)canShowCrowdfundingPromo;
+ (void)didShowDonationPage;
+ (void)resetDonations;

@end

typedef NS_ENUM(NSUInteger, MWMSettingsPowerManagement) {
  MWMSettingsPowerManagementNone,
  MWMSettingsPowerManagementNormal,
  MWMSettingsPowerManagementEconomyMedium,
  MWMSettingsPowerManagementEconomyMaximum,
  MWMSettingsPowerManagementAuto,
};

NS_SWIFT_NAME(Settings)
@interface MWMSettings : NSObject <PromoManager>

+ (NSString *)osmUserName;

+ (BOOL)autoDownloadEnabled;
+ (void)setAutoDownloadEnabled:(BOOL)autoDownloadEnabled;

+ (MWMUnits)measurementUnits;
+ (void)setMeasurementUnits:(MWMUnits)measurementUnits;

+ (BOOL)zoomButtonsEnabled;
+ (void)setZoomButtonsEnabled:(BOOL)zoomButtonsEnabled;

+ (MWMPlacement)bookmarksTextPlacement;
+ (void)setBookmarksTextPlacement:(MWMPlacement)placement;

+ (BOOL)compassCalibrationEnabled;
+ (void)setCompassCalibrationEnabled:(BOOL)compassCalibrationEnabled;

+ (MWMTheme)theme;
+ (void)setTheme:(MWMTheme)theme;

+ (BOOL)routingDisclaimerApproved;
+ (void)setRoutingDisclaimerApproved;

+ (NSString *)spotlightLocaleLanguageId;
+ (void)setSpotlightLocaleLanguageId:(NSString *)spotlightLocaleLanguageId;

+ (BOOL)largeFontSize;
+ (void)setLargeFontSize:(BOOL)largeFontSize;

+ (BOOL)transliteration;
+ (void)setTransliteration:(BOOL)transliteration;

+ (BOOL)map3dBuildingsEnabled;
+ (void)setMap3dBuildingsEnabled:(BOOL)enabled;

+ (BOOL)perspectiveViewEnabled;
+ (void)setPerspectiveViewEnabled:(BOOL)enabled;

+ (BOOL)autoZoomEnabled;
+ (void)setAutoZoomEnabled:(BOOL)enabled;

+ (BOOL)searchHistoryEnabled;
+ (void)setSearchHistoryEnabled:(BOOL)enabled;

+ (MWMSettingsPowerManagement)powerManagement;
+ (void)setPowerManagement:(MWMSettingsPowerManagement)powerManagement;
+ (BOOL)isPowerManagementMaximum;

+ (BOOL)isTrackWarningAlertShown;
+ (void)setTrackWarningAlertShown:(BOOL)shown;

+ (NSString *)donateUrl;
+ (BOOL)isNY;

+ (BOOL)isShowDownloadedRegions;
+ (void)setShowDownloadedRegions:(BOOL)isEnabled;

+ (MWMNetworkPolicyPermission)mobileInternetPermission;
+ (void)setMobileInternetPermission:(MWMNetworkPolicyPermission)permission;

+ (BOOL)iCLoudSynchronizationEnabled;
+ (void)setICLoudSynchronizationEnabled:(BOOL)iCLoudSyncEnabled;

+ (void)initializeLogging;
+ (BOOL)isFileLoggingEnabled;
+ (void)setFileLoggingEnabled:(BOOL)fileLoggingEnabled;
+ (uint64_t)logFileSize;

+ (BOOL)didShowICloudSynchronizationEnablingAlert;
+ (void)setICloudSynchronizationEnablingAlertShown;

@end
