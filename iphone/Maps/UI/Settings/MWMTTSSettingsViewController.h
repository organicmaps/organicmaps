#import "MWMTableViewController.h"

#include <string>
#include <vector>
#include <utility>

@interface MWMTTSSettingsViewController : MWMTableViewController

- (void)setAdditionalTTSLanguage:(std::pair<std::string, std::string> const &)l;

- (std::vector<std::pair<std::string, std::string>> const &)languages;

@end
