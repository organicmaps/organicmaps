#import "MWMViewController.h"

#include "storage/index.hpp"

@interface MWMMapDownloadDialog : UIView

+ (void)pauseAutoDownload:(BOOL)pause;

+ (instancetype)dialogForController:(MWMViewController *)controller;

- (void)processViewportCountryEvent:(storage::TCountryId const &)countryId;

@end
