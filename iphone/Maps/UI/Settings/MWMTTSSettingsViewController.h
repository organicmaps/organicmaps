#import "MWMTableViewController.h"

#include <string>
#include <utility>
#include <vector>

@interface MWMTTSSettingsViewController : MWMTableViewController

- (void)setAdditionalTTSLanguage:(std::pair<std::string, std::string> const &)l;

- (std::vector<std::pair<std::string, std::string>> const &)languages;

@end
