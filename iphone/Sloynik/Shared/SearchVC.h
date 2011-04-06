#import <UIKit/UIKit.h>

@class ArticleVC;

typedef struct SloynikData SloynikData;

@interface SearchVC : UIViewController
<UISearchBarDelegate, UITableViewDelegate, UITableViewDataSource>
{
  SloynikData * m_pSloynikData;
  UISearchBar * searchBar;
	UITableView * resultsView;
}

@property (nonatomic, retain) IBOutlet UISearchBar * searchBar;
@property (nonatomic, retain) IBOutlet UITableView * resultsView;

- (void)willShowArticleVC:(ArticleVC *) articleVC;
- (void)onEmptySearch;

@end
