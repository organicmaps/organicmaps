NS_SWIFT_NAME(Settings)
@interface MWMSettings : NSObject

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

+ (BOOL)isTrackWarningAlertShown;
+ (void)setTrackWarningAlertShown:(BOOL)shown;

+ (NSString *)donateUrl;
+ (BOOL)isNY;

+ (BOOL)iCLoudSynchronizationEnabled;
+ (void)setICLoudSynchronizationEnabled:(BOOL)iCLoudSyncEnabled;

+ (void)initializeLogging;
+ (BOOL)isFileLoggingEnabled;
+ (void)setFileLoggingEnabled:(BOOL)fileLoggingEnabled;

@end
