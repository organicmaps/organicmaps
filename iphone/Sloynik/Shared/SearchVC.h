#import <UIKit/UIKit.h>

@class ArticleVC;

typedef struct SloynikData SloynikData;

@interface SearchVC : UIViewController
<UISearchBarDelegate, UITableViewDelegate, UITableViewDataSource>
{
  SloynikData * m_pSloynikData;
  UISearchBar * searchBar;
	UITableView * resultsView;
  UIBarButtonItem * menuButton;
  ArticleVC * articleVC;
  CGRect initFrame;
}

@property (nonatomic, retain) UISearchBar * searchBar;
@property (nonatomic, retain) UITableView * resultsView;
@property (nonatomic, retain) UIBarButtonItem * menuButton;
@property (nonatomic, retain) ArticleVC * articleVC;
@property (nonatomic) CGRect initFrame;

- (void)menuButtonPressed;
- (void)willShowArticle;
- (void)showArticle;

@end
