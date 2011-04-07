#import <UIKit/UIKit.h>

@interface ArticleVC : UIViewController
<UINavigationBarDelegate, UIWebViewDelegate, UIGestureRecognizerDelegate>
{
  UIWebView * webView;         // Web view that displays an article
  UINavigationBar * navBar;    // Navigation bar
  UINavigationItem * navSearch;
  UINavigationItem * navArticle;
  UIPinchGestureRecognizer * pinchGestureRecognizer;
  NSString * articleFormat;
  CGRect m_webViewFrame;
  unsigned int m_articleId;
  double m_fontScale;
  double m_fontScaleOnPinchStart;
}

@property (nonatomic, retain) UIWebView * webView;
@property (nonatomic, retain) UINavigationBar * navBar;
@property (nonatomic, retain) UINavigationItem * navSearch;
@property (nonatomic, retain) UINavigationItem * navArticle;
@property (nonatomic, retain) UIPinchGestureRecognizer * pinchGestureRecognizer;
@property (nonatomic, retain) NSString * articleFormat;

- (id)initWithNibName:(NSString *)nibName bundle:(NSBundle *)nibBundle;
- (void)setArticleById:(unsigned int)articleId;

@end
