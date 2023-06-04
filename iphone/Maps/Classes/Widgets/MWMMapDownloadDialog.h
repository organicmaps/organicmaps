#import "MWMViewController.h"

#include "storage/storage_defines.hpp"

@class MapViewController;

@interface MWMMapDownloadDialog : UIView

+ (instancetype)dialogForController:(MapViewController *)controller;

- (void)processViewportCountryEvent:(storage::CountryId const &)countryId;

@end
