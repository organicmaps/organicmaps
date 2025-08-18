#import "MWMTextToSpeechObserver.h"

@interface MWMTextToSpeech : NSObject

+ (MWMTextToSpeech *)tts;
+ (BOOL)isTTSEnabled;
+ (void)setTTSEnabled:(BOOL)enabled;
+ (BOOL)isStreetNamesTTSEnabled;
+ (void)setStreetNamesTTSEnabled:(BOOL)enabled;
+ (NSString *)savedLanguage;

+ (void)addObserver:(id<MWMTextToSpeechObserver>)observer;
+ (void)removeObserver:(id<MWMTextToSpeechObserver>)observer;

+ (void)applicationDidBecomeActive;

@property(nonatomic) BOOL active;
- (void)setNotificationsLocale:(NSString *)locale;
- (void)playTurnNotifications:(NSArray<NSString *> *)turnNotifications;
- (void)playWarningSound;
- (void)play:(NSString *)text;

- (instancetype)init __attribute__((unavailable("call +tts instead")));
- (instancetype)copy __attribute__((unavailable("call +tts instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +tts instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone __attribute__((unavailable("call +tts instead")));
+ (instancetype)new __attribute__((unavailable("call +tts instead")));

@end
