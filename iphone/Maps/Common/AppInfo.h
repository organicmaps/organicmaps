typedef NS_ENUM(NSInteger, MWMOpenGLDriver) {
  MWMOpenGLDriverRegular,
  MWMOpenGLDriverMetalPre103, // iOS 10..10.3
  MWMOpenGLDriverMetal
};

@interface AppInfo : NSObject

+ (instancetype)sharedInfo;
- (instancetype)init __attribute__((unavailable("init is not available")));

@property(nonatomic, readonly) NSString * countryCode;
@property(nonatomic, readonly) NSString * uniqueId;
@property(nonatomic, readonly) NSString * bundleVersion;
@property(nonatomic, readonly) NSString * buildNumber;
@property(nonatomic, readonly) NSUUID * advertisingId;
@property(nonatomic, readonly) NSString * inputLanguage;
@property(nonatomic, readonly) NSString * twoLetterInputLanguage;
@property(nonatomic, readonly) NSString * languageId;
@property(nonatomic, readonly) NSString * twoLetterLanguageId;
@property(nonatomic, readonly) NSDate * buildDate;
@property(nonatomic, readonly) NSString * deviceModel;
@property(nonatomic, readonly) MWMOpenGLDriver openGLDriver;
@property(nonatomic, readonly) BOOL canMakeCalls;

@end
