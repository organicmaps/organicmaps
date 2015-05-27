
#import <UIKit/UIKit.h>
#import "ViewController.h"
#import "LocationManager.h"
#import "LocationButton.h"
#import "SearchView.h"
#import "LocationPredictor.h"

#include "../../geometry/point2d.hpp"
#include "../../geometry/rect2d.hpp"
#include "../../indexer/map_style.hpp"

namespace search { struct AddressInfo; }

@class MWMMapViewControlsManager;
@class ShareActionSheet;

@interface MapViewController : ViewController <LocationObserver, UIAlertViewDelegate, UIPopoverControllerDelegate>
{
  enum Action
	{
		NOTHING,
		DRAGGING,
		SCALING
	} m_CurrentAction;

	bool m_isSticking;
	size_t m_StickyThreshold;
	m2::PointD m_Pt1, m_Pt2;

  /// Temporary solution to improve long touch detection.
  m2::PointD m_touchDownPoint;

  CGPoint m_popoverPos;
  
  LocationPredictor * m_predictor;
}

- (void)setupMeasurementSystem;

// called when app is terminated by system
- (void)onTerminate;
- (void)onEnterForeground;
- (void)onEnterBackground;

- (void)dismissPopover;

- (void)setMapStyle:(MapStyle)mapStyle;

@property (nonatomic) UIPopoverController * popoverVC;
@property (nonatomic, readonly) BOOL apiMode;
@property (nonatomic) SearchView * searchView;
@property (nonatomic) ShareActionSheet * shareActionSheet;
- (void)setApiMode:(BOOL)apiMode animated:(BOOL)animated;
@property (nonatomic, readonly) MWMMapViewControlsManager * controlsManager;
@property (nonatomic) m2::PointD restoreRouteDestination;

@end
