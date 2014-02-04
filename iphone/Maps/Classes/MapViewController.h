
#import <UIKit/UIKit.h>
#import "LocationManager.h"
#import "LocationButton.h"
#import "SideToolbar.h"

#include "../../geometry/point2d.hpp"
#include "../../geometry/rect2d.hpp"


namespace search { struct AddressInfo; }

@interface MapViewController : UIViewController <LocationObserver, UIAlertViewDelegate, UIPopoverControllerDelegate>
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
}

- (void)setupMeasurementSystem;

// called when app is terminated by system
- (void)onTerminate;
- (void)onEnterForeground;
- (void)onEnterBackground;

- (IBAction)onMyPositionClicked:(id)sender;

- (void)showSearchResultAsBookmarkAtMercatorPoint:(m2::PointD const &)pt withInfo:(search::AddressInfo const &)info;
- (void)showBalloonWithCategoryIndex:(int)cat andBookmarkIndex:(int)bm;

- (void)prepareForApi;
- (void)clearApiMode;
- (BOOL)shouldShowNavBar;

- (void)dismissPopover;

@property (nonatomic) BOOL isApiMode;

@property (weak, nonatomic) IBOutlet UIView * zoomButtonsView;
@property (nonatomic) SideToolbar * sideToolbar;
@property (nonatomic, strong) UIPopoverController * popoverVC;
@property (nonatomic) UIView * fadeView;

@end
