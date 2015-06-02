
#import <UIKit/UIKit.h>
#import "SearchBar.h"

typedef NS_ENUM(NSUInteger, SearchViewState) {
  SearchViewStateHidden,
  SearchViewStateResults,
  SearchViewStateAlpha,
  SearchViewStateFullscreen,
};

@class MWMMapViewControlsManager;

@protocol SearchViewDelegate <NSObject>

- (void)searchViewWillEnterState:(SearchViewState)state;

@end

@interface SearchView : UIView

@property (nonatomic) SearchBar * searchBar;

- (void)setState:(SearchViewState)state animated:(BOOL)animated withCallback:(BOOL)withCallback;

@property (weak, nonatomic) id <SearchViewDelegate> delegate;
@property (readonly, nonatomic) SearchViewState state;

@end
