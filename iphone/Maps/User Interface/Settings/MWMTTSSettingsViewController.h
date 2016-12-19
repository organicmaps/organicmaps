#import "MWMTableViewController.h"

#include "std/string.hpp"
#include "std/utility.hpp"

@interface MWMTTSSettingsViewController : MWMTableViewController

- (void)setAdditionalTTSLanguage:(pair<string, string> const &)l;

@end
