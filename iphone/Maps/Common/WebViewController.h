#import <WebKit/WebKit.h>
#import "MWMViewController.h"
#import "MWMTypes.h"

@interface WebViewController : MWMViewController <WKNavigationDelegate>

@property (nonatomic) NSURL * _Nullable m_url;
@property (copy, nonatomic) NSString * _Nullable m_htmlText;
// Set to YES if external browser should be launched
@property (nonatomic) BOOL openInSafari;

- (instancetype _Nullable)initWithUrl:(NSURL * _Nonnull)url title:( NSString * _Nullable)title;
- (instancetype _Nullable)initWithHtml:(NSString * _Nonnull)htmlText
                               baseUrl:(NSURL * _Nullable)url
                                 title:(NSString * _Nullable)title;
- (instancetype _Nullable)initWithAuthURL:(NSURL * _Nonnull)url
                            onSuccessAuth:(MWMStringBlock _Nullable)success
                                onFailure:(MWMVoidBlock _Nullable)failure;
- (void)forward;
- (void)back;

@end
