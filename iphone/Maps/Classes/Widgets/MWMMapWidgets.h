#include "Framework.h"

@interface MWMMapWidgets : NSObject

+ (MWMMapWidgets *)widgetsManager;

- (void)setupWidgets:(Framework::DrapeCreationParams &)p;

- (void)resize:(CGSize)size;
- (void)updateAvailableArea:(CGRect)frame;

@end
