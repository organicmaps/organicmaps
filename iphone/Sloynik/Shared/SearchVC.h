#import <UIKit/UIKit.h>

@class ArticleVC;

typedef struct SloynikData SloynikData;

@interface SearchVC : UIViewController
<UISearchBarDelegate, UITableViewDelegate, UITableViewDataSource>
{
  SloynikData * m_pSloynikData;
  UISearchBar * searchBar;
	UITableView * resultsView;
  ArticleVC * articleVC;
}

@property (nonatomic, retain) IBOutlet UISearchBar * searchBar;
@property (nonatomic, retain) IBOutlet UITableView * resultsView;
@property (nonatomic, retain) ArticleVC * articleVC;

- (void)willShowArticleVC:(ArticleVC *) articleVC;
- (void)onEmptySearch;

@end
