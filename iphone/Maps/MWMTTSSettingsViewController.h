#import "MWMTableViewController.h"

#include "std/utility.hpp"

@interface MWMTTSSettingsViewController : MWMTableViewController

- (void)setAdditionalTTSLanguage:(pair<string, string> const &)l;

@end
