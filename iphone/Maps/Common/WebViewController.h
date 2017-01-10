#import "MWMViewController.h"

@interface WebViewController : MWMViewController <UIWebViewDelegate>

@property (nonatomic) NSURL * m_url;
@property (copy, nonatomic) NSString * m_htmlText;
// Set to YES if external browser should be launched
@property (nonatomic) BOOL openInSafari;

- (id)initWithUrl:(NSURL *)url andTitleOrNil:(NSString *)title;
- (id)initWithHtml:(NSString *)htmlText baseUrl:(NSURL *)url andTitleOrNil:(NSString *)title;

@end
