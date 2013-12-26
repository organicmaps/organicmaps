//
// Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MRCalendarManager.h"
#import <EventKit/EventKit.h>
#import "MPInstanceProvider.h"
#import "UIViewController+MPAdditions.h"
#import "MPLastResortDelegate.h"

@interface MRCalendarManager ()

@property (nonatomic, retain) EKEventEditViewController *eventEditViewController;
@property (nonatomic, retain) NSArray *acceptedDateFormatters;

- (EKEvent *)calendarEventWithParameters:(NSDictionary *)parameters
                              eventStore:(EKEventStore *)eventStore;
- (void)presentCalendarEditor:(EKEventEditViewController *)editor;
- (NSDate *)dateWithParameters:(NSDictionary *)parameters forKey:(NSString *)key;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRCalendarManager

@synthesize delegate = _delegate;
@synthesize eventEditViewController = _eventEditViewController;
@synthesize acceptedDateFormatters = _acceptedDateFormatters;

- (id)initWithDelegate:(id<MRCalendarManagerDelegate>)delegate
{
    if (self = [super init]) {
        _delegate = delegate;
        self.acceptedDateFormatters = @[[self dateFormatterForFormat:@"yyyy-MM-dd'T'HH:mmZZZ"],
                                        [self dateFormatterForFormat:@"yyyy-MM-dd'T'HH:mm:ssZZZ"]];
    }
    return self;
}

- (void)dealloc
{
    self.acceptedDateFormatters = nil;
    // XXX:
    [_eventEditViewController setEditViewDelegate:[MPLastResortDelegate sharedDelegate]];
    [_eventEditViewController release];
    [super dealloc];
}

#pragma mark - NSDateFormatterss

- (NSDateFormatter *)dateFormatterForFormat:(NSString *)format
{
    NSDateFormatter *formatter = [[[NSDateFormatter alloc] init] autorelease];
    [formatter setDateFormat:format];

    return formatter;
}

- (NSDate *)parseDateFromString:(NSString *)dateString
{
    NSDate *result = nil;

    for (NSDateFormatter *formatter in self.acceptedDateFormatters) {
        result = [formatter dateFromString:dateString];
        if (result != nil) {
            break;
        }
    }

    return result;
}

#pragma mark - Public

- (void)createCalendarEventWithParameters:(NSDictionary *)parameters
{
    if (!self.eventEditViewController) {
        self.eventEditViewController = [[MPInstanceProvider sharedProvider] buildEKEventEditViewControllerWithEditViewDelegate:self];
    }

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 60000
    if ([self.eventEditViewController.eventStore respondsToSelector:@selector(requestAccessToEntityType:completion:)]) {
        [self.eventEditViewController.eventStore requestAccessToEntityType:EKEntityTypeEvent completion:^(BOOL granted, NSError *error) {
            if (granted) {
                self.eventEditViewController.event = [self calendarEventWithParameters:parameters
                                                                            eventStore:self.eventEditViewController.eventStore];
                dispatch_async(dispatch_get_main_queue(), ^{
                    [self.delegate calendarManagerWillPresentCalendarEditor:self];
                    [self presentCalendarEditor:self.eventEditViewController];
                });
            } else {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [self.delegate calendarManager:self didFailToCreateCalendarEventWithErrorMessage:@"Could not create event because user has denied access to the calendar."];
                });
            }
        }];
    } else {
        self.eventEditViewController.event = [self calendarEventWithParameters:parameters
                                                                    eventStore:self.eventEditViewController.eventStore];
        [self.delegate calendarManagerWillPresentCalendarEditor:self];
        [self presentCalendarEditor:self.eventEditViewController];
    }
#else
    self.eventEditViewController.event = [self calendarEventWithParameters:parameters
                                                                eventStore:self.eventEditViewController.eventStore];
    [self.delegate calendarManagerWillPresentCalendarEditor:self];
    [self presentCalendarEditor:self.eventEditViewController];
#endif
}

#pragma mark - Internal

- (EKEvent *)calendarEventWithParameters:(NSDictionary *)parameters
                              eventStore:(EKEventStore *)eventStore
{
    EKEvent *event = [EKEvent eventWithEventStore:eventStore];
    event.title = [parameters objectForKey:@"description"];

    if ([parameters objectForKey:@"start"]) {
        event.startDate = [self dateWithParameters:parameters forKey:@"start"];
    }

    if ([parameters objectForKey:@"end"]) {
        event.endDate = [self dateWithParameters:parameters forKey:@"end"];
    }

    event.location = [parameters objectForKey:@"location"];
    event.notes = [parameters objectForKey:@"summary"];

    if ([parameters objectForKey:@"absoluteReminder"]) {
        [event addAlarm:[EKAlarm alarmWithAbsoluteDate:[self dateWithParameters:parameters forKey:@"absoluteReminder"]]];
    } else if ([parameters objectForKey:@"relativeReminder"]) {
        [event addAlarm:[EKAlarm alarmWithRelativeOffset:[[parameters objectForKey:@"relativeReminder"] doubleValue]]];
    }

    // XXX: We currently use 'interval' to signal whether there's a recurrence rule or not.
    // Should we use a more obvious flag?
    if ([parameters objectForKey:@"interval"]) {
        EKRecurrenceRule *rule = [self recurrenceRuleWithParameters:parameters];
        [event addRecurrenceRule:rule];
    }

    event.calendar = [eventStore defaultCalendarForNewEvents];
    NSString *transparency = [parameters objectForKey:@"transparency"];
    if ([transparency isEqualToString:@"opaque"]) {
        event.availability = EKEventAvailabilityBusy;
    } else if ([transparency isEqualToString:@"transparent"]) {
        event.availability = EKEventAvailabilityFree;
    }

    return event;
}

- (void)presentCalendarEditor:(EKEventEditViewController *)editor
{
    [[self.delegate viewControllerForPresentingCalendarEditor] mp_presentModalViewController:editor
                                                                                    animated:MP_ANIMATED];
}

- (NSDate *)dateWithParameters:(NSDictionary *)parameters forKey:(NSString *)key
{
    NSString *isoDateString = [parameters objectForKey:key];
    NSError *error = nil;
    NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:@"[-+]\\d\\d:"
                                                                           options:NSRegularExpressionCaseInsensitive
                                                                             error:&error];
    NSTextCheckingResult *regexResult = [regex firstMatchInString:isoDateString options:0
                                                            range:NSMakeRange(0, [isoDateString length])];
    NSString *timezone = [isoDateString substringWithRange:[regexResult range]];
    NSString *reformattedTimeZone = [timezone stringByReplacingOccurrencesOfString:@":" withString:@""];
    NSString *rfcDateString = [regex stringByReplacingMatchesInString:isoDateString
                                                              options:0
                                                                range:NSMakeRange(0, [isoDateString length])
                                                         withTemplate:reformattedTimeZone];
    NSDate *result = [self parseDateFromString:rfcDateString];
    return result;
}

#pragma mark - Recurrence Rules

- (EKRecurrenceRule *)recurrenceRuleWithParameters:(NSDictionary *)parameters {
    NSInteger interval = [[parameters objectForKey:@"interval"] integerValue];
    EKRecurrenceFrequency frequency = [self getFrequencyWithParameters:parameters];
    EKRecurrenceEnd *recurrenceEnd = [self getRecurrenceEndWithParameters:parameters];
    NSArray *daysOfTheWeek = [self getDaysOfTheWeekFromParameters:parameters];
    NSArray *daysInMonth = [self getDaysOfTheMonthFromParameters:parameters];
    NSArray *daysInYear = [self getDaysOfTheYearFromParameters:parameters];
    NSArray *monthsInYear = [self getMonthsOfTheYearFromParameters:parameters];

    return [[[EKRecurrenceRule alloc] initRecurrenceWithFrequency:frequency
                                                         interval:interval
                                                    daysOfTheWeek:daysOfTheWeek
                                                   daysOfTheMonth:daysInMonth
                                                  monthsOfTheYear:monthsInYear
                                                   weeksOfTheYear:nil // not in MRAID
                                                    daysOfTheYear:daysInYear
                                                     setPositions:nil
                                                              end:recurrenceEnd] autorelease];
}

- (EKRecurrenceFrequency)getFrequencyWithParameters:(NSDictionary *)parameters {
    NSString *frequencyStr = [parameters objectForKey:@"frequency"];
    if ([frequencyStr isEqualToString:@"weekly"]) {
        return EKRecurrenceFrequencyWeekly;
    } else if ([frequencyStr isEqualToString:@"monthly"]) {
        return EKRecurrenceFrequencyMonthly;
    } else if ([frequencyStr isEqualToString:@"yearly"]) {
        return EKRecurrenceFrequencyYearly;
    }
    return EKRecurrenceFrequencyDaily;
}

- (EKRecurrenceEnd *)getRecurrenceEndWithParameters:(NSDictionary *)parameters {
    EKRecurrenceEnd *recurrenceEnd = nil;

    if ([parameters objectForKey:@"expires"]) {
        NSDate *end = [NSDate dateWithTimeIntervalSince1970:[[parameters objectForKey:@"expires"] doubleValue]];
        recurrenceEnd = [EKRecurrenceEnd recurrenceEndWithEndDate:end];
    }

    return recurrenceEnd;
}

- (NSArray *)getDaysOfTheWeekFromParameters:(NSDictionary *)parameters {
    NSString *daysStr = [parameters objectForKey:@"daysInWeek"];
    NSArray *daysArray = [daysStr componentsSeparatedByString:@","];
    NSMutableArray *days = [NSMutableArray array];
    for (NSString *dayValue in daysArray) {
        @try {
            EKRecurrenceDayOfWeek *day = [EKRecurrenceDayOfWeek dayOfWeek:[dayValue integerValue]+1];
            [days addObject:day];
        } @catch (NSException *exception) {
            days = nil;
        }
    }

    return [NSArray arrayWithArray:days];
}

- (NSArray *)getTimeUnitsFromParameters:(NSDictionary *)parameters withKey:(NSString *)key {
    NSString *clientValueString = [parameters objectForKey:key];
    NSArray *clientValueArray = [clientValueString componentsSeparatedByString:@","];
    NSMutableArray *builder = [NSMutableArray array];
    for (NSString *value in clientValueArray) {
        NSInteger valueInteger = [value integerValue] > 0 ? [value integerValue] : [value integerValue]-1;
        NSNumber *number = [NSNumber numberWithInteger:valueInteger];
        [builder addObject:number];
    }

    return [NSArray arrayWithArray:builder];
}

- (NSArray *)getDaysOfTheMonthFromParameters:(NSDictionary *)parameters {
    return [self getTimeUnitsFromParameters:parameters withKey:@"daysInMonth"];
}

- (NSArray *)getDaysOfTheYearFromParameters:(NSDictionary *)parameters {
    return [self getTimeUnitsFromParameters:parameters withKey:@"daysInYear"];
}

- (NSArray *)getMonthsOfTheYearFromParameters:(NSDictionary *)parameters {
    NSString *monthsInYearStr = [parameters objectForKey:@"monthsInYear"];
    NSArray *monthsInYearArray = [monthsInYearStr componentsSeparatedByString:@","];
    NSMutableArray *monthsInYear = [NSMutableArray array];
    for (NSString *monthValue in monthsInYearArray) {
        NSNumber *monthNumber = [NSNumber numberWithInteger:[monthValue integerValue]];
        [monthsInYear addObject:monthNumber];
    }

    return [NSArray arrayWithArray:monthsInYear];
}

#pragma mark - <EKEventEditViewDelegate>

- (void)eventEditViewController:(EKEventEditViewController *)controller
          didCompleteWithAction:(EKEventEditViewAction)action
{
    if (action == EKEventEditViewActionSaved) {
        BOOL succeeded = [controller.eventStore saveEvent:controller.event span:EKSpanThisEvent error:nil];
        if (!succeeded) {
            [self.delegate calendarManager:self didFailToCreateCalendarEventWithErrorMessage:@"Failed to store created event in the event store."];
        }
    } else if (action == EKEventEditViewActionCanceled) {
        [self.delegate calendarManager:self didFailToCreateCalendarEventWithErrorMessage:@"Failed to create event because the user canceled the action."];
    }

    [controller mp_dismissModalViewControllerAnimated:MP_ANIMATED];
    [self.delegate calendarManagerDidDismissCalendarEditor:self];
}

@end
