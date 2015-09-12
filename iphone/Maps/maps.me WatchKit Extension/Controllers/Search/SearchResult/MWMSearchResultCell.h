#import <Foundation/Foundation.h>

@class WKInterfaceLabel;

@interface MWMSearchResultCell : NSObject

@property (weak, nonatomic, readonly) IBOutlet WKInterfaceLabel * titleLabel;
@property (weak, nonatomic, readonly) IBOutlet WKInterfaceLabel * categoryLabel;
@property (weak, nonatomic, readonly) IBOutlet WKInterfaceLabel * distanceLabel;

@end
