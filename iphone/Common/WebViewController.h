#import <UIKit/UIKit.h>

@interface WebViewController : UIViewController

@property (nonatomic) NSURL * m_url;
@property (nonatomic) NSString * m_htmlText;

- (id)initWithUrl:(NSURL *)url andTitleOrNil:(NSString *)title;
- (id)initWithHtml:(NSString *)htmlText baseUrl:(NSURL *)url andTitleOrNil:(NSString *)title;

@end
