//
//  PlacePreviewViewController.h
//  Maps
//
//  Created by Kirill on 04/06/2013.
//  Copyright (c) 2013 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>


namespace search { struct AddressInfo; }
namespace url_scheme { struct ApiPoint; }

typedef enum {APIPOINT, POI, MYPOSITION} Type;

@interface PlacePreviewViewController : UITableViewController <UIActionSheetDelegate, UIGestureRecognizerDelegate>
-(id)initWith:(search::AddressInfo const &)info point:(CGPoint)point;
-(id)initWithApiPoint:(url_scheme::ApiPoint const &)apiPoint;
-(id)initWithPoint:(CGPoint)point;
@end
