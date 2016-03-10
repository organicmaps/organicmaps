#import "MWMViewController.h"

#include "storage/index.hpp"

@interface MWMMapDownloadDialog : UIView

+ (instancetype)dialogForController:(MWMViewController *)controller;

- (void)processViewportCountryEvent:(storage::TCountryId const &)countryId;

@end
