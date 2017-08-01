#import "MWMTextToSpeech.h"

#include "std/string.hpp"
#include "std/vector.hpp"

@interface MWMTextToSpeech (CPP)

// Returns a list of available languages in the following format:
// * name in bcp47;
// * localized name;
- (vector<std::pair<string, string>>)availableLanguages;

@end

namespace tts
{
string translatedTwine(string const & twine);
}  // namespace tts
