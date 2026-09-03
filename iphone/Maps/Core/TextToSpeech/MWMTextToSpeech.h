#import <Foundation/Foundation.h>

#import "MWMTextToSpeechObserver.h"

NS_ASSUME_NONNULL_BEGIN

// Posted when voices may have been installed or removed, i.e. after the app returns from the
// background. Screens listing languages or voices should reload on it.
extern NSNotificationName const MWMTextToSpeechVoicesDidChangeNotification;

// A language the turn notifications can be generated in, e.g. "Deutsch" or "中文（普通话）". Identified by
// the internal code the notification texts are keyed by, so two regional variants of one language
// (en-GB and en-US) are the same MWMTTSLanguage, while zh-Hans and zh-Hant are not.
NS_SWIFT_NAME(TTSLanguage)
@interface MWMTTSLanguage : NSObject

@property(nonatomic, readonly) NSString * code;
@property(nonatomic, readonly) NSString * title;

+ (instancetype)languageWithCode:(NSString *)code title:(NSString *)title;

- (instancetype)init __attribute__((unavailable("call +languageWithCode:title: instead")));
+ (instancetype)new __attribute__((unavailable("call +languageWithCode:title: instead")));

@end

// The set a voice belongs to in Settings > Accessibility > VoiceOver > Speech. Most voices are
// Standard; the others are variously less suited to reading out directions.
typedef NS_ENUM(NSInteger, MWMTTSVoiceGroup) {
  MWMTTSVoiceGroupStandard,
  MWMTTSVoiceGroupEloquence,
  MWMTTSVoiceGroupNovelty,
} NS_SWIFT_NAME(TTSVoiceGroup);

// One of the system voices that can read out the notifications, e.g. "Samantha".
NS_SWIFT_NAME(TTSVoice)
@interface MWMTTSVoice : NSObject

@property(nonatomic, readonly) NSString * identifier;
// The name shown in the UI: the voice name, suffixed with the quality tier for the higher-quality
// Enhanced/Premium variants, e.g. "Samantha (Enhanced)".
@property(nonatomic, readonly) NSString * title;
// Localized name of the region the voice speaks for, e.g. "Ireland" for en-IE. Voices of one
// language differ mostly by accent, and their names alone do not convey it. Nil when the voice has
// no region, e.g. Cantonese "yue".
@property(nonatomic, readonly, nullable) NSString * region;
@property(nonatomic, readonly) MWMTTSVoiceGroup group;

+ (instancetype)voiceWithIdentifier:(NSString *)identifier
                              title:(NSString *)title
                             region:(NSString * _Nullable)region
                              group:(MWMTTSVoiceGroup)group;

- (instancetype)init __attribute__((unavailable("call +voiceWithIdentifier:title:region:group: instead")));
+ (instancetype)new __attribute__((unavailable("call +voiceWithIdentifier:title:region:group: instead")));

@end

@interface MWMTextToSpeech : NSObject

+ (MWMTextToSpeech * _Null_unspecified)tts;
+ (BOOL)isTTSEnabled;
+ (void)setTTSEnabled:(BOOL)enabled;
+ (BOOL)isStreetNamesTTSEnabled;
+ (void)setStreetNamesTTSEnabled:(BOOL)enabled;
// BCP-47 tag of the voice reading out the notifications, e.g. "en-GB". Use +currentLanguage for
// the language itself; this is the tag needed to look up the matching app resources.
+ (NSString * _Nullable)savedLanguage;
// Languages the notifications can be generated in that have at least one installed voice.
+ (NSArray<MWMTTSLanguage *> *)availableLanguages;
// The language the notifications are read in, nil when no voice is installed at all.
+ (MWMTTSLanguage * _Nullable)currentLanguage;
// Selects a language without pinning a voice, so the best installed one keeps being picked
// automatically as voices are added or removed.
+ (void)setLanguage:(MWMTTSLanguage *)language;
// Installed voices that read out `language`, one entry per voice name, sorted by name.
+ (NSArray<MWMTTSVoice *> *)voicesForLanguage:(MWMTTSLanguage *)language NS_SWIFT_NAME(voices(for:));
// The voice +setLanguage: would settle on, i.e. what the language sounds like today.
+ (MWMTTSVoice * _Nullable)bestVoiceForLanguage:(MWMTTSLanguage *)language NS_SWIFT_NAME(bestVoice(for:));
// The voice reading out the notifications: the saved one while it is installed and still reads the
// selected language, otherwise the best installed voice for it. Nil when none is installed.
+ (MWMTTSVoice * _Nullable)currentVoice;
// Selects the voice and, with it, the language it reads out.
+ (void)setVoice:(MWMTTSVoice *)voice;

+ (void)addObserver:(id<MWMTextToSpeechObserver>)observer;
+ (void)removeObserver:(id<MWMTextToSpeechObserver>)observer;

+ (void)applicationDidBecomeActive;

@property(nonatomic) BOOL active;
- (void)playTurnNotifications:(NSArray<NSString *> *)turnNotifications;
- (void)playWarningSound;

// Speaks a sample phrase with an arbitrary voice, leaving the voice used for the notifications
// alone. A preview waits for navigation speech; navigation speech interrupts a preview. `completion`
// runs on the main queue when the preview ends on its own, but not when -stopPreview ends it.
- (void)speakPreview:(NSString *)text
     voiceIdentifier:(NSString *)voiceIdentifier
          completion:(void (^_Nullable)(void))completion;
- (void)stopPreview;

- (instancetype)init __attribute__((unavailable("call +tts instead")));
- (instancetype)copy __attribute__((unavailable("call +tts instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +tts instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone __attribute__((unavailable("call +tts instead")));
+ (instancetype)new __attribute__((unavailable("call +tts instead")));

@end

NS_ASSUME_NONNULL_END
