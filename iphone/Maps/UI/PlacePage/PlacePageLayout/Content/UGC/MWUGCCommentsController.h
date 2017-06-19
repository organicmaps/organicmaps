#import "MWMTableViewController.h"

#include <vector>

namespace ugc
{
struct Review;
}

@interface MWUGCCommentsController : MWMTableViewController

+ (instancetype)instanceWithTitle:(NSString *)title comments:(std::vector<ugc::Review> const &)comments;

@end
