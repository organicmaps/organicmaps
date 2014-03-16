
#import <UIKit/UIKit.h>
#import "SearchActivityProtocol.h"
#import "SearchBar.h"

@interface SearchView : UIView <SearchActivityProtocol>

@property (nonatomic) SearchBar * searchBar;

@end
