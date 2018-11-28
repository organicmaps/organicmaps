#import "MWMMapViewControlsManager.h"
#import "MWMPlacePageButtonsProtocol.h"

@class MWMViewController;

@protocol MWMActionBarProtocol<NSObject>

- (void)routeFrom;
- (void)routeTo;
- (void)routeAddStop;
- (void)routeRemoveStop;

- (void)share;

- (void)addBookmark;
- (void)removeBookmark;

- (void)call;
- (void)book:(BOOL)isDecription;
- (void)searchBookingHotels;

- (void)openPartner;

- (void)apiBack;
- (void)downloadSelectedArea;

@end

struct FeatureID;

@protocol MWMFeatureHolder<NSObject>

- (FeatureID const &)featureId;

@end

namespace booking
{
struct HotelFacility;
}

@protocol MWMBookingInfoHolder<NSObject>

- (std::vector<booking::HotelFacility> const &)hotelFacilities;
- (NSString *)hotelName;

@end

@protocol MWMPlacePageProtocol<MWMActionBarProtocol, MWMPlacePageButtonsProtocol, MWMFeatureHolder, MWMBookingInfoHolder>

- (void)show:(place_page::Info const &)info;
- (void)showReview:(place_page::Info const &)info;
- (void)dismiss;
- (void)mwm_refreshUI;

@end
