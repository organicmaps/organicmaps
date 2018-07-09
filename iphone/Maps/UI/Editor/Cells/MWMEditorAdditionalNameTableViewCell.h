#import "MWMEditorCommon.h"
#import "MWMTableViewCell.h"

#include "indexer/editable_map_object.hpp"

@interface MWMEditorAdditionalNameTableViewCell : MWMTableViewCell

@property(nonatomic, readonly) NSInteger code;

- (void)configWithDelegate:(id<MWMEditorAdditionalName>)delegate
                  langCode:(NSInteger)langCode
                  langName:(NSString *)langName
                      name:(NSString *)name
              errorMessage:(NSString *)errorMessage
                   isValid:(BOOL)isValid
              keyboardType:(UIKeyboardType)keyboardType;

@end
