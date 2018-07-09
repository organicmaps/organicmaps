#import "MWMViewController.h"

#include "Framework.h"

@interface MWMAutoupdateController : MWMViewController

+ (instancetype)instanceWithPurpose:(Framework::DoAfterUpdate)todo;

@end
