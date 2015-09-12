#import <Foundation/Foundation.h>

@class WKInterfaceLabel, WKInterfaceImage;

@interface MWMCategoriesInterfaceCell : NSObject

@property (weak, nonatomic, readonly) IBOutlet WKInterfaceImage * icon;
@property (weak, nonatomic, readonly) IBOutlet WKInterfaceLabel * label;

@end
