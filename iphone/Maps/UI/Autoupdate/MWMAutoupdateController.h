#import "MWMViewController.h"

#include <CoreApi/Framework.h>

@interface MWMAutoupdateController : MWMViewController

+ (instancetype)instanceWithPurpose:(Framework::DoAfterUpdate)todo;

@end
