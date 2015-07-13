
#import "MWMNavigationDelegate.h"
#import "SearchBar.h"
#import <UIKit/UIKit.h>

#include "platform/country_defines.hpp"
#include "storage/index.hpp"

typedef NS_ENUM(NSUInteger, SearchViewState) {
  SearchViewStateHidden,
  SearchViewStateResults,
  SearchViewStateAlpha,
  SearchViewStateFullscreen,
};

@class MWMMapViewControlsManager;

@protocol SearchViewDelegate <NSObject>

@property (nonatomic, readonly) BOOL haveCurrentMap;

- (void)searchViewWillEnterState:(SearchViewState)state;
- (void)searchViewDidEnterState:(SearchViewState)state;

- (void)startMapDownload:(storage::TIndex const &)index type:(TMapOptions)type;
- (void)stopMapsDownload;
- (void)restartMapDownload:(storage::TIndex const &)index;

@end

@interface SearchView : UIView

@property (nonnull, nonatomic) SearchBar * searchBar;

- (void)setState:(SearchViewState)state animated:(BOOL)animated;
- (CGFloat)defaultSearchBarMinY;

@property (nonnull, weak, nonatomic) id <SearchViewDelegate, MWMNavigationDelegate> delegate;
@property (nonatomic, readonly) SearchViewState state;

@property (nonatomic, readonly) CGRect infoRect;

- (void)downloadProgress:(CGFloat)progress countryName:(nonnull NSString *)countryName;
- (void)downloadComplete;
- (void)downloadFailed;

@end
