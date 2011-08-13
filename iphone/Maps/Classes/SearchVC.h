#import <UIKit/UIKit.h>

#include "../../map/framework.hpp"

#include "../../std/vector.hpp"
#include "../../std/function.hpp"
#include "../../std/string.hpp"

namespace search { class Result; }

typedef function<void (string const &, SearchCallbackT)> SearchF;
typedef function<void (m2::RectD)> ShowRectF;

@interface SearchVC : UIViewController
    <UISearchBarDelegate, UITableViewDelegate, UITableViewDataSource>
{
  vector<search::Result> m_results;
}

- (id)initWithSearchFunc:(SearchF)s andShowRectFunc:(ShowRectF)r;

@end
