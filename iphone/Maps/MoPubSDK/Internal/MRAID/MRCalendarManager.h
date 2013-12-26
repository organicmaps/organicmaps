//
// Copyright (c) 2013 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <EventKitUI/EventKitUI.h>

@protocol MRCalendarManagerDelegate;

@interface MRCalendarManager : NSObject <EKEventEditViewDelegate>

@property (nonatomic, assign) NSObject<MRCalendarManagerDelegate> *delegate;

- (id)initWithDelegate:(NSObject<MRCalendarManagerDelegate> *)delegate;
- (void)createCalendarEventWithParameters:(NSDictionary *)parameters;

@end

@protocol MRCalendarManagerDelegate <NSObject>

@required
- (UIViewController *)viewControllerForPresentingCalendarEditor;
- (void)calendarManagerWillPresentCalendarEditor:(MRCalendarManager *)manager;
- (void)calendarManagerDidDismissCalendarEditor:(MRCalendarManager *)manager;
- (void)calendarManager:(MRCalendarManager *)manager
        didFailToCreateCalendarEventWithErrorMessage:(NSString *)message;

@end
