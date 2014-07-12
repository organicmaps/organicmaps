/* Copyright (c) 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//
//  GTLDateTime.m
//

#import "GTLDateTime.h"

@interface GTLDateTime ()

- (void)setFromDate:(NSDate *)date timeZone:(NSTimeZone *)tz;
- (void)setFromRFC3339String:(NSString *)str;

@property (nonatomic, retain, readwrite) NSTimeZone *timeZone;
@property (nonatomic, copy, readwrite) NSDateComponents *dateComponents;
@property (nonatomic, assign, readwrite) NSInteger milliseconds;

@property (nonatomic, assign, readwrite) BOOL hasTime;
@property (nonatomic, assign, readwrite) NSInteger offsetSeconds;
@property (nonatomic, assign, getter=isUniversalTime, readwrite) BOOL universalTime;

@end

static NSCharacterSet *gDashSet = nil;
static NSCharacterSet *gTSet = nil;
static NSCharacterSet *gColonSet = nil;
static NSCharacterSet *gPlusMinusZSet = nil;
static NSMutableDictionary *gCalendarsForTimeZones = nil;

@implementation GTLDateTime

// A note about milliseconds_:
// RFC 3339 has support for fractions of a second.  NSDateComponents is all
// NSInteger based, so it can't handle a fraction of a second.  NSDate is
// built on NSTimeInterval so it has sub-millisecond precision.  GTL takes
// the compromise of supporting the RFC's optional fractional second support
// by maintaining a number of milliseconds past what fits in the
// NSDateComponents.  The parsing and string conversions will include
// 3 decimal digits (hence milliseconds).  When going to a string, the decimal
// digits are only included if the milliseconds are non zero.

@dynamic date;
@dynamic calendar;
@dynamic RFC3339String;
@dynamic stringValue;
@dynamic timeZone;
@dynamic hasTime;

@synthesize dateComponents = dateComponents_,
            milliseconds = milliseconds_,
            offsetSeconds = offsetSeconds_,
            universalTime = isUniversalTime_;

+ (void)initialize {
  // Note that initialize is guaranteed by the runtime to be called in a
  // thread-safe manner.
  if (gDashSet == nil) {
    gDashSet = [[NSCharacterSet characterSetWithCharactersInString:@"-"] retain];
    gTSet = [[NSCharacterSet characterSetWithCharactersInString:@"Tt "] retain];
    gColonSet = [[NSCharacterSet characterSetWithCharactersInString:@":"] retain];
    gPlusMinusZSet = [[NSCharacterSet characterSetWithCharactersInString:@"+-zZ"] retain];

    gCalendarsForTimeZones = [[NSMutableDictionary alloc] init];
  }
}

+ (GTLDateTime *)dateTimeWithRFC3339String:(NSString *)str {
  if (str == nil) return nil;

  GTLDateTime *result = [[[self alloc] init] autorelease];
  [result setFromRFC3339String:str];
  return result;
}

+ (GTLDateTime *)dateTimeWithDate:(NSDate *)date timeZone:(NSTimeZone *)tz {
  if (date == nil) return nil;

  GTLDateTime *result = [[[self alloc] init] autorelease];
  [result setFromDate:date timeZone:tz];
  return result;
}

+ (GTLDateTime *)dateTimeForAllDayWithDate:(NSDate *)date {
  if (date == nil) return nil;

  GTLDateTime *result = [[[self alloc] init] autorelease];
  [result setFromDate:date timeZone:nil];
  result.hasTime = NO;
  return result;
}

+ (GTLDateTime *)dateTimeWithDateComponents:(NSDateComponents *)components {
  NSCalendar *cal = [[[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar] autorelease];
  NSDate *date = [cal dateFromComponents:components];
#if GTL_IPHONE
  NSTimeZone *tz = [components timeZone];
#else
  // NSDateComponents added timeZone: in Mac OS X 10.7.
  NSTimeZone *tz = nil;
  if ([components respondsToSelector:@selector(timeZone)]) {
    tz = [components timeZone];
  }
#endif
  return [self dateTimeWithDate:date timeZone:tz];
}

- (void)dealloc {
  [dateComponents_ release];
  [timeZone_ release];
  [super dealloc];
}

- (id)copyWithZone:(NSZone *)zone {
  // Object is immutable
  return [self retain];
}

// until NSDateComponent implements isEqual, we'll use this
- (BOOL)doesDateComponents:(NSDateComponents *)dc1
       equalDateComponents:(NSDateComponents *)dc2 {

  return [dc1 era] == [dc2 era]
          && [dc1 year] == [dc2 year]
          && [dc1 month] == [dc2 month]
          && [dc1 day] == [dc2 day]
          && [dc1 hour] == [dc2 hour]
          && [dc1 minute] == [dc2 minute]
          && [dc1 second] == [dc2 second]
          && [dc1 week] == [dc2 week]
          && [dc1 weekday] == [dc2 weekday]
          && [dc1 weekdayOrdinal] == [dc2 weekdayOrdinal];
}

- (BOOL)isEqual:(GTLDateTime *)other {

  if (self == other) return YES;
  if (![other isKindOfClass:[GTLDateTime class]]) return NO;

  BOOL areDateComponentsEqual = [self doesDateComponents:self.dateComponents
                                     equalDateComponents:other.dateComponents];
  NSTimeZone *tz1 = self.timeZone;
  NSTimeZone *tz2 = other.timeZone;
  BOOL areTimeZonesEqual = (tz1 == tz2 || (tz2 && [tz1 isEqual:tz2]));

  return self.offsetSeconds == other.offsetSeconds
    && self.isUniversalTime == other.isUniversalTime
    && self.milliseconds == other.milliseconds
    && areDateComponentsEqual
    && areTimeZonesEqual;
}

- (NSString *)description {
  return [NSString stringWithFormat:@"%@ %p: {%@}",
    [self class], self, self.RFC3339String];
}

- (NSTimeZone *)timeZone {
  if (timeZone_) {
    return timeZone_;
  }

  if (self.isUniversalTime) {
    NSTimeZone *ztz = [NSTimeZone timeZoneWithName:@"Universal"];
    return ztz;
  }

  NSInteger offsetSeconds = self.offsetSeconds;

  if (offsetSeconds != NSUndefinedDateComponent) {
    NSTimeZone *tz = [NSTimeZone timeZoneForSecondsFromGMT:offsetSeconds];
    return tz;
  }
  return nil;
}

- (void)setTimeZone:(NSTimeZone *)timeZone {
  [timeZone_ release];
  timeZone_ = [timeZone retain];

  if (timeZone) {
    NSInteger offsetSeconds = [timeZone secondsFromGMTForDate:self.date];
    self.offsetSeconds = offsetSeconds;
  } else {
    self.offsetSeconds = NSUndefinedDateComponent;
  }
}

- (NSCalendar *)calendarForTimeZone:(NSTimeZone *)tz {
  NSCalendar *cal = nil;
  @synchronized(gCalendarsForTimeZones) {
    id tzKey = (tz ? tz : [NSNull null]);
    cal = [gCalendarsForTimeZones objectForKey:tzKey];
    if (cal == nil) {
      cal = [[[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar] autorelease];
      if (tz) {
        [cal setTimeZone:tz];
      }
      [gCalendarsForTimeZones setObject:cal forKey:tzKey];
    }
  }
  return cal;
}

- (NSCalendar *)calendar {
  NSTimeZone *tz = self.timeZone;
  return [self calendarForTimeZone:tz];
}

- (NSDate *)date {
  NSDateComponents *dateComponents = self.dateComponents;
  NSTimeInterval extraMillisecondsAsSeconds = 0.0;
  NSCalendar *cal;

  if (!self.hasTime) {
    // We're not keeping track of a time, but NSDate always is based on
    // an absolute time. We want to avoid returning an NSDate where the
    // calendar date appears different from what was used to create our
    // date-time object.
    //
    // We'll make a copy of the date components, setting the time on our
    // copy to noon GMT, since that ensures the date renders correctly for
    // any time zone.
    NSDateComponents *noonDateComponents = [[dateComponents copy] autorelease];
    [noonDateComponents setHour:12];
    [noonDateComponents setMinute:0];
    [noonDateComponents setSecond:0];
    dateComponents = noonDateComponents;
    
    NSTimeZone *gmt = [NSTimeZone timeZoneWithName:@"Universal"];
    cal = [self calendarForTimeZone:gmt];
  } else {
    cal = self.calendar;

    // Add in the fractional seconds that don't fit into NSDateComponents.
    extraMillisecondsAsSeconds = ((NSTimeInterval)self.milliseconds) / 1000.0;
  }

  NSDate *date = [cal dateFromComponents:dateComponents];

  // Add in any milliseconds that didn't fit into the dateComponents.
  if (extraMillisecondsAsSeconds > 0.0) {
#if GTL_IPHONE || (MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_5)
    date = [date dateByAddingTimeInterval:extraMillisecondsAsSeconds];
#else
    date = [date addTimeInterval:extraMillisecondsAsSeconds];
#endif
  }

  return date;
}

- (NSString *)stringValue {
  return self.RFC3339String;
}

- (NSString *)RFC3339String {
  NSDateComponents *dateComponents = self.dateComponents;
  NSInteger offset = self.offsetSeconds;

  NSString *timeString = @""; // timeString like "T15:10:46-08:00"

  if (self.hasTime) {

    NSString *timeOffsetString; // timeOffsetString like "-08:00"

    if (self.isUniversalTime) {
     timeOffsetString = @"Z";
    } else if (offset == NSUndefinedDateComponent) {
      // unknown offset is rendered as -00:00 per
      // http://www.ietf.org/rfc/rfc3339.txt section 4.3
      timeOffsetString = @"-00:00";
    } else {
      NSString *sign = @"+";
      if (offset < 0) {
        sign = @"-";
        offset = -offset;
      }
      timeOffsetString = [NSString stringWithFormat:@"%@%02ld:%02ld",
        sign, (long)(offset/(60*60)) % 24, (long)(offset / 60) % 60];
    }

    NSString *fractionalSecondsString = @"";
    if (self.milliseconds > 0.0) {
      fractionalSecondsString = [NSString stringWithFormat:@".%03ld", (long)self.milliseconds];
    }

    timeString = [NSString stringWithFormat:@"T%02ld:%02ld:%02ld%@%@",
      (long)[dateComponents hour], (long)[dateComponents minute],
      (long)[dateComponents second], fractionalSecondsString, timeOffsetString];
  }

  // full dateString like "2006-11-17T15:10:46-08:00"
  NSString *dateString = [NSString stringWithFormat:@"%04ld-%02ld-%02ld%@",
    (long)[dateComponents year], (long)[dateComponents month],
    (long)[dateComponents day], timeString];

  return dateString;
}

- (void)setFromDate:(NSDate *)date timeZone:(NSTimeZone *)tz {
  NSCalendar *cal = [self calendarForTimeZone:tz];

  NSUInteger const kComponentBits = (NSYearCalendarUnit | NSMonthCalendarUnit
    | NSDayCalendarUnit | NSHourCalendarUnit | NSMinuteCalendarUnit
    | NSSecondCalendarUnit);

  NSDateComponents *components = [cal components:kComponentBits fromDate:date];
  self.dateComponents = components;

  // Extract the fractional seconds.
  NSTimeInterval asTimeInterval = [date timeIntervalSince1970];
  NSTimeInterval worker = asTimeInterval - trunc(asTimeInterval);
  self.milliseconds = (NSInteger)round(worker * 1000.0);
  
  self.universalTime = NO;

  NSInteger offset = NSUndefinedDateComponent;

  if (tz) {
    offset = [tz secondsFromGMTForDate:date];

    if (offset == 0 && [tz isEqualToTimeZone:[NSTimeZone timeZoneWithName:@"Universal"]]) {
      self.universalTime = YES;
    }
  }
  self.offsetSeconds = offset;

  // though offset seconds are authoritative, we'll retain the time zone
  // since we can't regenerate it reliably from just the offset
  timeZone_ = [tz retain];
}

- (void)setFromRFC3339String:(NSString *)str {

  NSInteger year = NSUndefinedDateComponent;
  NSInteger month = NSUndefinedDateComponent;
  NSInteger day = NSUndefinedDateComponent;
  NSInteger hour = NSUndefinedDateComponent;
  NSInteger minute = NSUndefinedDateComponent;
  NSInteger sec = NSUndefinedDateComponent;
  NSInteger milliseconds = 0;
  double secDouble = -1.0;
  NSString* sign = nil;
  NSInteger offsetHour = 0;
  NSInteger offsetMinute = 0;

  if ([str length] > 0) {
    NSScanner* scanner = [NSScanner scannerWithString:str];
    // There should be no whitespace, so no skip characters.
    [scanner setCharactersToBeSkipped:nil];

    // for example, scan 2006-11-17T15:10:46-08:00
    //                or 2006-11-17T15:10:46Z
    if (// yyyy-mm-dd
        [scanner scanInteger:&year] &&
        [scanner scanCharactersFromSet:gDashSet intoString:NULL] &&
        [scanner scanInteger:&month] &&
        [scanner scanCharactersFromSet:gDashSet intoString:NULL] &&
        [scanner scanInteger:&day] &&
        // Thh:mm:ss
        [scanner scanCharactersFromSet:gTSet intoString:NULL] &&
        [scanner scanInteger:&hour] &&
        [scanner scanCharactersFromSet:gColonSet intoString:NULL] &&
        [scanner scanInteger:&minute] &&
        [scanner scanCharactersFromSet:gColonSet intoString:NULL] &&
        [scanner scanDouble:&secDouble]) {

      // At this point we got secDouble, pull it apart.
      sec = (NSInteger)secDouble;
      double worker = secDouble - ((double)sec);
      milliseconds = (NSInteger)round(worker * 1000.0);

      // Finish parsing, now the offset info.
      if (// Z or +hh:mm
          [scanner scanCharactersFromSet:gPlusMinusZSet intoString:&sign] &&
          [scanner scanInteger:&offsetHour] &&
          [scanner scanCharactersFromSet:gColonSet intoString:NULL] &&
          [scanner scanInteger:&offsetMinute]) {
      }
    }
  }

  NSDateComponents *dateComponents = [[[NSDateComponents alloc] init] autorelease];
  [dateComponents setYear:year];
  [dateComponents setMonth:month];
  [dateComponents setDay:day];
  [dateComponents setHour:hour];
  [dateComponents setMinute:minute];
  [dateComponents setSecond:sec];

  self.dateComponents = dateComponents;
  self.milliseconds = milliseconds;
  
  // determine the offset, like from Z, or -08:00:00.0

  self.timeZone = nil;

  NSInteger totalOffset = NSUndefinedDateComponent;
  self.universalTime = NO;

  if ([sign caseInsensitiveCompare:@"Z"] == NSOrderedSame) {

    self.universalTime = YES;
    totalOffset = 0;

  } else if (sign != nil) {

    totalOffset = (60 * offsetMinute) + (60 * 60 * offsetHour);

    if ([sign isEqual:@"-"]) {

      if (totalOffset == 0) {
        // special case: offset of -0.00 means undefined offset
        totalOffset = NSUndefinedDateComponent;
      } else {
        totalOffset *= -1;
      }
    }
  }

  self.offsetSeconds = totalOffset;
}

- (BOOL)hasTime {
  NSDateComponents *dateComponents = self.dateComponents;

  BOOL hasTime = ([dateComponents hour] != NSUndefinedDateComponent
                  && [dateComponents minute] != NSUndefinedDateComponent);

  return hasTime;
}

- (void)setHasTime:(BOOL)shouldHaveTime {

  // we'll set time values to zero or NSUndefinedDateComponent as appropriate
  BOOL hadTime = self.hasTime;

  if (shouldHaveTime && !hadTime) {
    [dateComponents_ setHour:0];
    [dateComponents_ setMinute:0];
    [dateComponents_ setSecond:0];
    milliseconds_ = 0;
    offsetSeconds_ = NSUndefinedDateComponent;
    isUniversalTime_ = NO;

  } else if (hadTime && !shouldHaveTime) {
    [dateComponents_ setHour:NSUndefinedDateComponent];
    [dateComponents_ setMinute:NSUndefinedDateComponent];
    [dateComponents_ setSecond:NSUndefinedDateComponent];
    milliseconds_ = 0;
    offsetSeconds_ = NSUndefinedDateComponent;
    isUniversalTime_ = NO;
    self.timeZone = nil;
  }
}


@end
