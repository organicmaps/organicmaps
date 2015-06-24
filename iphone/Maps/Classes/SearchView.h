
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
- (void)searchViewDidEnterState:(SearchViewState)state;

@end

@interface SearchView : UIView

@property (nonatomic) SearchBar * searchBar;

- (void)setState:(SearchViewState)state animated:(BOOL)animated;
- (CGFloat)defaultSearchBarMinY;

@property (weak, nonatomic) id <SearchViewDelegate> delegate;
@property (nonatomic, readonly) SearchViewState state;

@property (nonatomic, readonly) CGRect infoRect;

@end
