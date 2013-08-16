#import <Foundation/Foundation.h>

namespace url_scheme { struct ApiPoint; }

@interface MWMApi : NSObject
+(NSURL *)getBackUrl:(url_scheme::ApiPoint const &)apiPoint;
+(void)openAppWithPoint:(url_scheme::ApiPoint const &)apiPoint;
+(BOOL)canOpenApiUrl:(url_scheme::ApiPoint const &)apiPoint;
@end
