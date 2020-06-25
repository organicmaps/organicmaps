/*
 * Copyright 2018 Google
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

#import <Foundation/Foundation.h>

@protocol FIRAnalyticsInteropListener;

NS_ASSUME_NONNULL_BEGIN

/// Block typedef callback parameter to getUserPropertiesWithCallback:.
typedef void (^FIRAInteropUserPropertiesCallback)(NSDictionary<NSString *, id> *userProperties);

/// Connector for bridging communication between Firebase SDKs and FirebaseAnalytics API.
@protocol FIRAnalyticsInterop

/// Sets user property when trigger event is logged. This API is only available in the SDK.
- (void)setConditionalUserProperty:(NSDictionary<NSString *, id> *)conditionalUserProperty;

/// Clears user property if set.
- (void)clearConditionalUserProperty:(NSString *)userPropertyName
                           forOrigin:(NSString *)origin
                      clearEventName:(NSString *)clearEventName
                clearEventParameters:(NSDictionary<NSString *, NSString *> *)clearEventParameters;

/// Returns currently set user properties.
- (NSArray<NSDictionary<NSString *, NSString *> *> *)conditionalUserProperties:(NSString *)origin
                                                            propertyNamePrefix:
                                                                (NSString *)propertyNamePrefix;

/// Returns the maximum number of user properties.
- (NSInteger)maxUserProperties:(NSString *)origin;

/// Returns the user properties to a callback function.
- (void)getUserPropertiesWithCallback:(FIRAInteropUserPropertiesCallback)callback;

/// Logs events.
- (void)logEventWithOrigin:(NSString *)origin
                      name:(NSString *)name
                parameters:(nullable NSDictionary<NSString *, id> *)parameters;

/// Sets user property.
- (void)setUserPropertyWithOrigin:(NSString *)origin name:(NSString *)name value:(id)value;

/// Registers an Analytics listener for the given origin.
- (void)registerAnalyticsListener:(id<FIRAnalyticsInteropListener>)listener
                       withOrigin:(NSString *)origin;

/// Unregisters an Analytics listener for the given origin.
- (void)unregisterAnalyticsListenerWithOrigin:(NSString *)origin;

@end

NS_ASSUME_NONNULL_END
