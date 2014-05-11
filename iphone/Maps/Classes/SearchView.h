
#import <UIKit/UIKit.h>
#import "SearchBar.h"

typedef NS_ENUM(NSUInteger, SearchViewState) {
  SearchViewStateHidden,
  SearchViewStateResults,
  SearchViewStateAlpha,
  SearchViewStateFullscreen,
};

@interface SearchView : UIView

@property (nonatomic) SearchBar * searchBar;

- (void)setState:(SearchViewState)state animated:(BOOL)animated withCallback:(BOOL)withCallback;
@property (readonly, nonatomic) SearchViewState state;

@end
