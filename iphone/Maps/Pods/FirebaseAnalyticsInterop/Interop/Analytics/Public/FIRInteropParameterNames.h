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

/// @file FIRInteropParameterNames.h
///
/// Predefined event parameter names used by Firebase. This file is a subset of the
/// FirebaseAnalytics FIRParameterNames.h public header.
///
/// The origin of your traffic, such as an Ad network (for example, google) or partner (urban
/// airship). Identify the advertiser, site, publication, etc. that is sending traffic to your
/// property. Highly recommended (NSString).
/// <pre>
///     NSDictionary *params = @{
///       kFIRParameterSource : @"InMobi",
///       // ...
///     };
/// </pre>
static NSString *const kFIRIParameterSource NS_SWIFT_NAME(AnalyticsParameterSource) = @"source";

/// The advertising or marketing medium, for example: cpc, banner, email, push. Highly recommended
/// (NSString).
/// <pre>
///     NSDictionary *params = @{
///       kFIRParameterMedium : @"email",
///       // ...
///     };
/// </pre>
static NSString *const kFIRIParameterMedium NS_SWIFT_NAME(AnalyticsParameterMedium) = @"medium";

/// The individual campaign name, slogan, promo code, etc. Some networks have pre-defined macro to
/// capture campaign information, otherwise can be populated by developer. Highly Recommended
/// (NSString).
/// <pre>
///     NSDictionary *params = @{
///       kFIRParameterCampaign : @"winter_promotion",
///       // ...
///     };
/// </pre>
static NSString *const kFIRIParameterCampaign NS_SWIFT_NAME(AnalyticsParameterCampaign) =
    @"campaign";

/// Message identifier.
static NSString *const kFIRIParameterMessageIdentifier = @"_nmid";

/// Message name.
static NSString *const kFIRIParameterMessageName = @"_nmn";

/// Message send time.
static NSString *const kFIRIParameterMessageTime = @"_nmt";

/// Message device time.
static NSString *const kFIRIParameterMessageDeviceTime = @"_ndt";

/// Topic message.
static NSString *const kFIRIParameterTopic = @"_nt";

/// Stores the message_id of the last notification opened by the app.
static NSString *const kFIRIUserPropertyLastNotification = @"_ln";
