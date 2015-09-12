#import <UIKit/UIKit.h>
#import "MWMTextView.h"

@interface UILabel (RuntimeAttributes)
@property (nonatomic) NSString * localizedText;
@end

@interface MWMTextView (RuntimeAttributes)
@property (nonatomic) NSString * localizedPlaceholder;
@end

@interface UITextField (RuntimeAttributes)
@property (nonatomic) NSString * localizedPlaceholder;
@end
