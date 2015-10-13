#include "Framework.h"

@interface MWMMapWidgets : NSObject

@property (nonatomic) BOOL fullScreen;
@property (nonatomic) CGFloat leftBound;
@property (nonatomic) CGFloat bottomBound;

- (void)setupWidgets:(Framework::DrapeCreationParams &)p;
- (void)resize:(CGSize)size;

@end
