//
//  MPNativeAdRenderer.h
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@protocol MPNativeAdAdapter;
@protocol MPNativeAdRendererSettings;
@class MPNativeAdRendererConfiguration;

/**
 *  Provide an implementation of this handler for your renderer settings.
 *
 *  @param maximumWidth Max width of the ad expected to be rendered as.
 *
 *  @return Size of the view as rendered given the maximum width desired.
 */
typedef CGSize (^MPNativeViewSizeHandler)(CGFloat maximumWidth);

/**
 * The MoPub SDK has a concept of native ad renderer that allows you to render the ad however you want. It also gives you the
 * ability to expose configurable properties through renderer settings objects to the application that influence how you render
 * your native custom event's view.
 *
 * Your renderer should implement this protocol. Your renderer is responsible for rendering the network's ad data into a view
 * when `-retrieveViewWithAdapter:error:` is called. Your renderer will be asked to render the native ad if your renderer configuration
 * indicates that your renderer supports your specific native ad network.
 *
 * Finally, your renderer will live as long as the ad adapter so you may store data in your renderer if necessary.
 */
@protocol MPNativeAdRenderer <NSObject>

@required

/**
 * You must construct and return an MPNativeAdRendererConfiguration object specific for your renderer. You must
 * set all the properties on the configuration object.
 *
 * @param rendererSettings Application defined settings that you should store in the configuration object that you
 * construct.
 *
 * @return A configuration object that allows the MoPub SDK to instantiate your renderer with the application
 * settings and for the supported ad types.
 */
+ (MPNativeAdRendererConfiguration *)rendererConfigurationWithRendererSettings:(id<MPNativeAdRendererSettings>)rendererSettings;

/**
 * This is the init method that will be called when the MoPub SDK initializes your renderer.
 *
 * @param rendererSettings The renderer settings object that corresponds to your renderer.
 */
- (instancetype)initWithRendererSettings:(id<MPNativeAdRendererSettings>)rendererSettings;

/**
 * You must return a native ad view when `-retrieveViewWithAdapter:error:` is called. Ideally, you should create a native view
 * each time this is called as holding onto the view may end up consuming a lot of memory when many ads are being shown.
 * However, it is OK to hold a strong reference to the view if you must.
 *
 * @param adapter Your custom event's adapter class that contains the network specific data necessary to render the ad to
 * a view.
 * @param error If you can't construct a view for whatever reason, you must fill in this error object.
 *
 * @return If successful, the method will return a native view presenting the ad. If it
 * is unsuccessful at retrieving a view, it will return nil and create
 * an error object for the error parameter.
 */
- (UIView *)retrieveViewWithAdapter:(id<MPNativeAdAdapter>)adapter error:(NSError **)error;

@optional

/**
 * The viewSizeHandler is used to allow the app to configure its native ad view size
 * given a maximum width when using ad placer solutions. This is not called when the
 * app is manually integrating native ads.
 *
 * You should obtain the renderer's viewSizeHandler from the settings object in
 * `-initWithRendererSettings:`.
 */
@property (nonatomic, readonly) MPNativeViewSizeHandler viewSizeHandler;

/**
 * The MoPubSDK will notify your renderer when your native ad's view has moved in
 * the hierarchy. superview will be nil if the native ad's view has been removed
 * from the view hierarchy.
 *
 * The view your renderer creates is attached to another view before being added
 * to the view hierarchy. As a result, the superview argument will not be the renderer's ad view's superview.
 *
 * @param superview The app's view that contains the native ad view. There is an
 * intermediate view between the renderer's ad view and the app's view.
 */
- (void)adViewWillMoveToSuperview:(UIView *)superview;

/**
 *
 * The MoPubSDK will call this method when the user has tapped the ad and will
 * invoke the clickthrough action.
 *
 */
- (void)nativeAdTapped;

@end
