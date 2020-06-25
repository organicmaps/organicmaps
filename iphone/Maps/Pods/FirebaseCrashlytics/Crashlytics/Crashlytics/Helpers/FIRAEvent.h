// Copyright 2019 Google
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#import <Foundation/Foundation.h>

@class FIRAPBEvent;

/// An application event.
@interface FIRAEvent : NSObject <NSCopying>

/// Origin of this event (eg "app" or "auto").
@property(nonatomic, readonly) NSString *origin;

/// Name of this event.
@property(nonatomic, readonly) NSString *name;

/// Timestamp of when this event was fired.
@property(nonatomic, readonly) NSTimeInterval timestamp;

/// Timestamp of the previous time an event with this name was fired, if any.
@property(nonatomic, readonly) NSTimeInterval previousTimestamp;

/// The event's parameters as {NSString : NSString} or {NSString : NSNumber}.
@property(nonatomic, readonly) NSDictionary *parameters;

/// Indicates whether the event has the conversion parameter. Setting to YES adds the conversion
/// parameter if not already present. Setting to NO removes the conversion parameter and adds an
/// error.
@property(nonatomic, getter=isConversion) BOOL conversion;

/// Indicates whether the event has the real-time parameter. Setting to YES adds the real-time
/// parameter if not already present. Setting to NO removes the real-time parameter.
@property(nonatomic, getter=isRealtime) BOOL realtime;

/// Indicates whether the event has debug parameter. Setting to YES adds the debug parameter if
/// not already present. Setting to NO removes the debug parameter.
@property(nonatomic, getter=isDebug) BOOL debug;

/// The populated FIRAPBEvent for proto.
@property(nonatomic, readonly) FIRAPBEvent *protoEvent;

/// Creates an event with the given parameters. Parameters will be copied and normalized. Returns
/// nil if the name does not meet length requirements.
/// If |parameters| contains the "_o" parameter, its value will be overwritten with the value of
/// |origin|.
- (instancetype)initWithOrigin:(NSString *)origin
                      isPublic:(BOOL)isPublic
                          name:(NSString *)name
                     timestamp:(NSTimeInterval)timestamp
             previousTimestamp:(NSTimeInterval)previousTimestamp
                    parameters:(NSDictionary *)parameters NS_DESIGNATED_INITIALIZER;

- (instancetype)init NS_UNAVAILABLE;

/// Returns a new event object with the given previousTimestamp.
- (instancetype)copyWithPreviousTimestamp:(NSTimeInterval)previousTimestamp;

/// Returns a new event object with the new parameters.
- (instancetype)copyWithParameters:(NSDictionary *)parameters;

/// Returns YES if all parameters in screenParameters were added to the event object. Returns NO if
/// screenParameters is nil/empty or the event already contains any of the screen parameter keys.
/// Performs internal validation on the screen parameter values and converts them to FIRAValue
/// objects if they aren't already. screenParameters should be a dictionary of
/// { NSString : NSString | NSNumber } or { NSString : FIRAValue }.
- (BOOL)addScreenParameters:(NSDictionary *)screenParameters;

@end
