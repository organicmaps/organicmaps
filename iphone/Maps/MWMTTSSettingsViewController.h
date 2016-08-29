#import "MWMTableViewController.h"

#include "std/utility.hpp"
#include "std/string.hpp"

@interface MWMTTSSettingsViewController : MWMTableViewController

- (void)setAdditionalTTSLanguage:(pair<string, string> const &)l;

@end
