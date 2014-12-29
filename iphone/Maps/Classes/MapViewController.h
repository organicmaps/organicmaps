
#import <UIKit/UIKit.h>
#import "ViewController.h"
#import "LocationManager.h"
#import "LocationButton.h"
#import "BottomMenu.h"
#import "SearchView.h"
#import "LocationPredictor.h"

#include "../../geometry/point2d.hpp"
#include "../../geometry/rect2d.hpp"


namespace search { struct AddressInfo; }

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

- (IBAction)onMyPositionClicked:(id)sender;

- (void)dismissPopover;

@property (nonatomic) UIView * zoomButtonsView;
@property (nonatomic, strong) UIPopoverController * popoverVC;
@property (nonatomic) BottomMenu * bottomMenu;
@property (nonatomic, readonly) BOOL apiMode;
@property (nonatomic) SearchView * searchView;
- (void)setApiMode:(BOOL)apiMode animated:(BOOL)animated;

@end
