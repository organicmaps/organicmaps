#import <UIKit/UIKit.h>

@interface WebViewController : UIViewController
{
  NSURL * m_url;
}

@property (nonatomic, retain) NSURL * m_url;

- (id) initWithUrl: (NSURL *)url andTitleOrNil:(NSString *)title;

@end
