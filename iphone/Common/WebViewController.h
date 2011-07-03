#import <UIKit/UIKit.h>

@interface WebViewController : UIViewController
{
  NSURL * m_url;
  NSString * m_htmlText;
}

@property (nonatomic, retain) NSURL * m_url;
@property (nonatomic, retain) NSString * m_htmlText;

- (id) initWithUrl: (NSURL *)url andTitleOrNil:(NSString *)title;
- (id) initWithHtml: (NSString *)htmlText
            baseUrl:(NSURL *)url 
      andTitleOrNil:(NSString *)title;

@end
