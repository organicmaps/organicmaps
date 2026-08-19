@class MWMMapWidgets;

NS_ASSUME_NONNULL_BEGIN

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
@interface EAGLView : UIView

@property(nonatomic) MWMMapWidgets * widgetsManager;

@property(nonatomic, readonly) BOOL drapeEngineCreated;
@property(nonatomic, readonly) CGSize pixelSize;
@property(nonatomic, readonly) BOOL graphicContextInitialized;
/// A one-shot callback reserved for a deferred CarPlay visual-scale update.
@property(nonatomic, copy, nullable) void (^graphicContextDidInitializeHandler)(void);

- (void)createDrapeEngine;
- (void)deallocateNative;
- (void)setPresentAvailable:(BOOL)available;
/// Takes a screen content scale factor (UIScreen.nativeScale, CPWindow.traitCollection.displayScale),
/// not a visual scale: the conversion is done here, exactly like on the engine creation.
- (void)updateVisualScaleWithContentScaleFactor:(CGFloat)contentScaleFactor;

@end

NS_ASSUME_NONNULL_END
