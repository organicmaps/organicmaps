#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, MWMWatchEventInfoRequest)
{
  MWMWatchEventInfoRequestMoveWritableDir
};

@interface NSDictionary (MWMWatchEventInfo)

@property (nonatomic, readonly) MWMWatchEventInfoRequest watchEventInfoRequest;

@end

@interface NSMutableDictionary (MWMWatchEventInfo)

@property (nonatomic) MWMWatchEventInfoRequest watchEventInfoRequest;

@end
