#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageRoute : NSObject

/// Short route identifier, e.g. "12A" or "S10".
@property(nonatomic, readonly) NSString * ref;
/// Human-readable start of the route (from the relation's "from" tag). May be empty.
@property(nonatomic, readonly) NSString * from;
/// Human-readable end of the route (from the relation's "to" tag). May be empty.
@property(nonatomic, readonly) NSString * to;
/// OSM relation id — passed back to C++ Framework::ShowRouteTransit(relId).
@property(nonatomic, readonly) uint32_t relId;
/// The relation's own color; nil when the relation has no color tag.
@property(nonatomic, readonly, nullable) UIColor * color;

+ (instancetype)routeWithRef:(NSString *)ref
                        from:(NSString *)from
                          to:(NSString *)to
                       relId:(uint32_t)relId
                       color:(nullable UIColor *)color;

@end

NS_ASSUME_NONNULL_END
