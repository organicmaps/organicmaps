#import <UIKit/UIKit.h>

@interface ArticleVC : UIViewController
<UINavigationBarDelegate, UIWebViewDelegate, UIGestureRecognizerDelegate>
{
  UIViewController * searchVC; // Search controller.
  UIWebView * webView;         // Web view that displays an article.
  UINavigationBar * navBar;    // Navigation bar.
  UINavigationItem * navSearch;
  UINavigationItem * navArticle;
  UISwipeGestureRecognizer * swipeLeftGestureRecognizer;
  UISwipeGestureRecognizer * swipeRightGestureRecognizer;
  UIPinchGestureRecognizer * pinchGestureRecognizer;
  CGRect m_webViewFrame;
  unsigned int m_articleId;
  double m_fontScale;
  double m_fontScaleOnPinchStart;
}

@property (nonatomic, assign) UIViewController * searchVC; // "assign" to avoid circular references.
@property (nonatomic, retain) UIWebView * webView;
@property (nonatomic, retain) UINavigationBar * navBar;
@property (nonatomic, retain) UINavigationItem * navSearch;
@property (nonatomic, retain) UINavigationItem * navArticle;
@property (nonatomic, retain) UISwipeGestureRecognizer * swipeLeftGestureRecognizer;
@property (nonatomic, retain) UISwipeGestureRecognizer * swipeRightGestureRecognizer;
@property (nonatomic, retain) UIPinchGestureRecognizer * pinchGestureRecognizer;

- (void)setArticleById:(unsigned int)articleId;

@end
