#import <AVFoundation/AVFoundation.h>
#import "MWMTextToSpeechObserver.h"

@interface MWMTextToSpeech : NSObject

+ (MWMTextToSpeech *)tts;
+ (BOOL)isTTSEnabled;
+ (void)setTTSEnabled:(BOOL)enabled;
+ (BOOL)isStreetNamesTTSEnabled;
+ (void)setStreetNamesTTSEnabled:(BOOL)enabled;
+ (NSString *)savedLanguage;
+ (NSString *)savedVoiceIdentifier;
+ (void)setSavedVoiceIdentifier:(NSString *)voiceIdentifier;
+ (NSArray<AVSpeechSynthesisVoice *> *)availableVoicesForLanguage:(NSString *)language;
// The name shown in the UI for a voice: its display name, suffixed with the quality tier for the
// higher-quality Enhanced/Premium variants (e.g. "Samantha (Enhanced)"). Returns nil for a nil voice.
+ (NSString *)displayNameForVoice:(AVSpeechSynthesisVoice *)voice;

+ (void)addObserver:(id<MWMTextToSpeechObserver>)observer;
+ (void)removeObserver:(id<MWMTextToSpeechObserver>)observer;

+ (void)applicationDidBecomeActive;

@property(nonatomic) BOOL active;
- (void)setNotificationsLocale:(NSString *)locale;
- (void)setVoiceIdentifier:(NSString *)voiceIdentifier;
// The voice currently used for playback: the saved voice if set, otherwise the best automatically
// selected voice for the current language. May be nil when no voice is available.
- (AVSpeechSynthesisVoice *)currentVoice;
- (void)playTurnNotifications:(NSArray<NSString *> *)turnNotifications;
- (void)playWarningSound;
- (void)play:(NSString *)text;

- (instancetype)init __attribute__((unavailable("call +tts instead")));
- (instancetype)copy __attribute__((unavailable("call +tts instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +tts instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone __attribute__((unavailable("call +tts instead")));
+ (instancetype)new __attribute__((unavailable("call +tts instead")));

@end
