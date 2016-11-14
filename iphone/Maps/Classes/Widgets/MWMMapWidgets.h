#include "Framework.h"

@interface MWMMapWidgets : NSObject

+ (MWMMapWidgets *)widgetsManager;

@property (nonatomic) BOOL fullScreen;
@property (nonatomic) CGFloat leftBound;
@property (nonatomic) CGFloat bottomBound;

- (void)setupWidgets:(Framework::DrapeCreationParams &)p;
- (void)resize:(CGSize)size;

- (void)layoutWidgets;

@end
