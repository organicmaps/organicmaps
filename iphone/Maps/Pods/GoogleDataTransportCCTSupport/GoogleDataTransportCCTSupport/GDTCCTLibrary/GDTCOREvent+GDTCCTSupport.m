/*
 * Copyright 2020 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import "GDTCCTLibrary/Public/GDTCOREvent+GDTCCTSupport.h"

#import <GoogleDataTransport/GDTCORConsoleLogger.h>

NSString *const GDTCCTNeedsNetworkConnectionInfo = @"needs_network_connection_info";

NSString *const GDTCCTNetworkConnectionInfo = @"network_connection_info";

NSString *const GDTCCTEventCodeInfo = @"event_code_info";

@implementation GDTCOREvent (GDTCCTSupport)

- (void)setNeedsNetworkConnectionInfoPopulated:(BOOL)needsNetworkConnectionInfoPopulated {
  if (!needsNetworkConnectionInfoPopulated) {
    if (!self.customBytes) {
      return;
    }

    // Make sure we don't destroy the eventCode data, if any is present.
    @try {
      NSError *error;
      NSMutableDictionary *bytesDict =
          [[NSJSONSerialization JSONObjectWithData:self.customBytes options:0
                                             error:&error] mutableCopy];
      if (error) {
        GDTCORLogDebug(@"Error when setting an event's event_code: %@", error);
        return;
      }
      NSNumber *eventCode = bytesDict[GDTCCTEventCodeInfo];
      if (eventCode != nil) {
        self.customBytes =
            [NSJSONSerialization dataWithJSONObject:@{GDTCCTEventCodeInfo : eventCode}
                                            options:0
                                              error:&error];
      }
    } @catch (NSException *exception) {
      GDTCORLogDebug(@"Error when setting the event for needs_network_connection_info: %@",
                     exception);
    }
  } else {
    @try {
      NSError *error;
      NSMutableDictionary *bytesDict;
      if (self.customBytes) {
        bytesDict = [[NSJSONSerialization JSONObjectWithData:self.customBytes
                                                     options:0
                                                       error:&error] mutableCopy];
        if (error) {
          GDTCORLogDebug(@"Error when setting an even'ts event_code: %@", error);
          return;
        }
      } else {
        bytesDict = [[NSMutableDictionary alloc] init];
      }
      [bytesDict setObject:@YES forKey:GDTCCTNeedsNetworkConnectionInfo];
      self.customBytes = [NSJSONSerialization dataWithJSONObject:bytesDict options:0 error:&error];
    } @catch (NSException *exception) {
      GDTCORLogDebug(@"Error when setting the event for needs_network_connection_info: %@",
                     exception);
    }
  }
}

- (BOOL)needsNetworkConnectionInfoPopulated {
  if (self.customBytes) {
    @try {
      NSError *error;
      NSDictionary *bytesDict = [NSJSONSerialization JSONObjectWithData:self.customBytes
                                                                options:0
                                                                  error:&error];
      return bytesDict && !error && [bytesDict[GDTCCTNeedsNetworkConnectionInfo] boolValue];
    } @catch (NSException *exception) {
      GDTCORLogDebug(@"Error when checking the event for needs_network_connection_info: %@",
                     exception);
    }
  }
  return NO;
}

- (void)setNetworkConnectionInfoData:(NSData *)networkConnectionInfoData {
  @try {
    NSError *error;
    NSString *dataString = [networkConnectionInfoData base64EncodedStringWithOptions:0];
    if (dataString != nil) {
      NSMutableDictionary *bytesDict;
      if (self.customBytes) {
        bytesDict = [[NSJSONSerialization JSONObjectWithData:self.customBytes
                                                     options:0
                                                       error:&error] mutableCopy];
        if (error) {
          GDTCORLogDebug(@"Error when setting an even'ts event_code: %@", error);
          return;
        }
      } else {
        bytesDict = [[NSMutableDictionary alloc] init];
      }
      [bytesDict setObject:dataString forKey:GDTCCTNetworkConnectionInfo];
      self.customBytes = [NSJSONSerialization dataWithJSONObject:bytesDict options:0 error:&error];
      if (error) {
        self.customBytes = nil;
        GDTCORLogDebug(@"Error when setting an event's network_connection_info: %@", error);
      }
    }
  } @catch (NSException *exception) {
    GDTCORLogDebug(@"Error when setting an event's network_connection_info: %@", exception);
  }
}

- (nullable NSData *)networkConnectionInfoData {
  if (self.customBytes) {
    @try {
      NSError *error;
      NSDictionary *bytesDict = [NSJSONSerialization JSONObjectWithData:self.customBytes
                                                                options:0
                                                                  error:&error];
      NSString *base64Data = bytesDict[GDTCCTNetworkConnectionInfo];
      NSData *networkConnectionInfoData = [[NSData alloc] initWithBase64EncodedString:base64Data
                                                                              options:0];
      if (error) {
        GDTCORLogDebug(@"Error when getting an event's network_connection_info: %@", error);
        return nil;
      } else {
        return networkConnectionInfoData;
      }
    } @catch (NSException *exception) {
      GDTCORLogDebug(@"Error when getting an event's network_connection_info: %@", exception);
    }
  }
  return nil;
}

- (NSNumber *)eventCode {
  if (self.customBytes) {
    @try {
      NSError *error;
      NSDictionary *bytesDict = [NSJSONSerialization JSONObjectWithData:self.customBytes
                                                                options:0
                                                                  error:&error];
      NSString *eventCodeString = bytesDict[GDTCCTEventCodeInfo];

      if (!eventCodeString) {
        return nil;
      }

      NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
      formatter.numberStyle = NSNumberFormatterDecimalStyle;
      NSNumber *eventCode = [formatter numberFromString:eventCodeString];

      if (error) {
        GDTCORLogDebug(@"Error when getting an event's network_connection_info: %@", error);
        return nil;
      } else {
        return eventCode;
      }
    } @catch (NSException *exception) {
      GDTCORLogDebug(@"Error when getting an event's event_code: %@", exception);
    }
  }
  return nil;
}

- (void)setEventCode:(NSNumber *)eventCode {
  if (eventCode == nil) {
    if (!self.customBytes) {
      return;
    }

    NSError *error;
    NSMutableDictionary *bytesDict = [[NSJSONSerialization JSONObjectWithData:self.customBytes
                                                                      options:0
                                                                        error:&error] mutableCopy];
    if (error) {
      GDTCORLogDebug(@"Error when setting an event's event_code: %@", error);
      return;
    }

    [bytesDict removeObjectForKey:GDTCCTEventCodeInfo];
    self.customBytes = [NSJSONSerialization dataWithJSONObject:bytesDict options:0 error:&error];
    if (error) {
      self.customBytes = nil;
      GDTCORLogDebug(@"Error when setting an event's event_code: %@", error);
      return;
    }
    return;
  }

  @try {
    NSMutableDictionary *bytesDict;
    NSError *error;
    if (self.customBytes) {
      bytesDict = [[NSJSONSerialization JSONObjectWithData:self.customBytes options:0
                                                     error:&error] mutableCopy];
      if (error) {
        GDTCORLogDebug(@"Error when setting an event's event_code: %@", error);
        return;
      }
    } else {
      bytesDict = [[NSMutableDictionary alloc] init];
    }

    NSString *eventCodeString = [eventCode stringValue];
    if (eventCodeString == nil) {
      return;
    }

    [bytesDict setObject:eventCodeString forKey:GDTCCTEventCodeInfo];

    self.customBytes = [NSJSONSerialization dataWithJSONObject:bytesDict options:0 error:&error];
    if (error) {
      self.customBytes = nil;
      GDTCORLogDebug(@"Error when setting an event's network_connection_info: %@", error);
      return;
    }

  } @catch (NSException *exception) {
    GDTCORLogDebug(@"Error when getting an event's network_connection_info: %@", exception);
  }
}

@end
