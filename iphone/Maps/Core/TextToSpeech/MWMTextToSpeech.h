#import <Foundation/Foundation.h>

#import "MWMTextToSpeechObserver.h"

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(TTSLanguage)
@interface MWMTTSLanguage : NSObject

@property(nonatomic, readonly) NSString * bcp47;
@property(nonatomic, readonly) NSString * title;

+ (instancetype)languageWithBcp47:(NSString *)bcp47 title:(NSString *)title;

- (instancetype)init __attribute__((unavailable("call +languageWithBcp47:title: instead")));
+ (instancetype)new __attribute__((unavailable("call +languageWithBcp47:title: instead")));

@end

@interface MWMTextToSpeech : NSObject

+ (MWMTextToSpeech * _Null_unspecified)tts;
+ (BOOL)isTTSEnabled;
+ (void)setTTSEnabled:(BOOL)enabled;
+ (BOOL)isStreetNamesTTSEnabled;
+ (void)setStreetNamesTTSEnabled:(BOOL)enabled;
+ (NSString * _Nullable)savedLanguage;
+ (NSArray<MWMTTSLanguage *> *)preferredLanguages;
+ (NSArray<MWMTTSLanguage *> *)availableLanguages;
+ (void)setNotificationsLanguage:(MWMTTSLanguage *)language;
+ (void)playRandomTestString;

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

NS_ASSUME_NONNULL_END
