#import <Foundation/Foundation.h>

@protocol MWMNavigationViewProtocol <NSObject>

- (void)navigationDashBoardDidUpdate;
- (void)routePreviewDidChangeFrame:(CGRect)newFrame;

@end
