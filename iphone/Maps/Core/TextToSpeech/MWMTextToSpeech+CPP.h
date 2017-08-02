#import "MWMTextToSpeech.h"

#include <string>
#include <vector>

@interface MWMTextToSpeech (CPP)

// Returns a list of available languages in the following format:
// * name in bcp47;
// * localized name;
- (std::vector<std::pair<std::string, std::string>>)availableLanguages;

@end

namespace tts
{
std::string translatedTwine(std::string const & twine);
}  // namespace tts
