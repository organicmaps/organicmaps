#import "MWMTableViewController.h"

#include "indexer/editable_map_object.hpp"

@protocol MWMEditorAdditionalNamesProtocol <NSObject>

- (void)addAdditionalName:(NSInteger)languageIndex;

@end

@interface MWMEditorAdditionalNamesTableViewController : MWMTableViewController

- (void)configWithDelegate:(id<MWMEditorAdditionalNamesProtocol>)delegate
                      name:(StringUtf8Multilang const &)name
additionalSkipLanguageCodes:(std::vector<NSInteger>)additionalSkipLanguageCodes;

@end
