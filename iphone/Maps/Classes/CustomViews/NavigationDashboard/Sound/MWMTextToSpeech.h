#include "std/string.hpp"
#include "std/vector.hpp"

@interface MWMTextToSpeech : NSObject

+ (instancetype)tts;
+ (void)activateAudioSession;
// Returns a list of available languages in the following format:
// * name in bcp47;
// * localized name;
- (vector<std::pair<string, string>>)availableLanguages;
- (NSString *)savedLanguage;
- (void)setNotificationsLocale:(NSString *)locale;
- (BOOL)isNeedToEnable;
- (void)setNeedToEnable:(BOOL)need;
- (BOOL)isEnable;
- (void)enable;
- (void)disable;
- (void)playTurnNotifications;

- (instancetype)init __attribute__((unavailable("call tts instead")));
- (instancetype)copy __attribute__((unavailable("call tts instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call tts instead")));
+ (instancetype)alloc __attribute__((unavailable("call tts instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone __attribute__((unavailable("call tts instead")));
+ (instancetype)new __attribute__((unavailable("call tts instead")));

@end

namespace tts
{

string bcp47ToTwineLanguage(NSString const * bcp47LangName);
string translatedTwine(string const & twine);

} // namespace tts
