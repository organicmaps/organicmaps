#include <CoreApi/Framework.h>

@interface MWMMapWidgets : NSObject

+ (MWMMapWidgets *)widgetsManager;

- (void)setupWidgets:(Framework::DrapeCreationParams &)p;

- (void)resize:(CGSize)size;
- (void)updateAvailableArea:(CGRect)frame;
- (void)updateLayout:(CGRect)frame;
- (void)updateLayoutForAvailableArea;

@end
