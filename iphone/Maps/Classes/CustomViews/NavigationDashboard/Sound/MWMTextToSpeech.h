#include "std/string.hpp"
#include "std/vector.hpp"

@interface MWMTextToSpeech : NSObject

+ (instancetype)tts;
+ (BOOL)isTTSEnabled;
+ (void)setTTSEnabled:(BOOL)enabled;
+ (NSString *)savedLanguage;

+ (NSString *)ttsStatusNotificationKey;

@property (nonatomic) BOOL active;
// Returns a list of available languages in the following format:
// * name in bcp47;
// * localized name;
- (vector<std::pair<string, string>>)availableLanguages;
- (void)setNotificationsLocale:(NSString *)locale;
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

string translatedTwine(string const & twine);

} // namespace tts
