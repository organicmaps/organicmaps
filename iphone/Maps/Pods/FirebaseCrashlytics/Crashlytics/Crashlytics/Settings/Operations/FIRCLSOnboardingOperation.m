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

#import "FIRCLSOnboardingOperation.h"

#import "FIRCLSByteUtility.h"
#import "FIRCLSConstants.h"
#import "FIRCLSFABNetworkClient.h"
#import "FIRCLSLogger.h"
#import "FIRCLSMachO.h"
#import "FIRCLSMultipartMimeStreamEncoder.h"
#import "FIRCLSSettings.h"
#import "FIRCLSURLBuilder.h"

// The SPIv1/v2 onboarding flow looks something like this:
// - get settings
//   - settings says we're good, nothing to do
//   - settings says update
//     - do an update
//   - settings says new
//     - do a create
//     - get settings again (and do *not* take action after that)

NSString *const FIRCLSOnboardingErrorDomain = @"FIRCLSOnboardingErrorDomain";

typedef NS_ENUM(NSInteger, FIRCLSOnboardingError) {
  FIRCLSOnboardingErrorMultipartMimeConfiguration
};

@interface FIRCLSOnboardingOperation ()

@property(nonatomic) BOOL shouldCreate;
@property(nonatomic, readonly) FIRCLSApplicationIdentifierModel *appIdentifierModel;
@property(nonatomic, readonly) NSString *appEndpoint;
@property(nonatomic, readonly, unsafe_unretained) id<FIRCLSOnboardingOperationDelegate> delegate;
@property(nonatomic, weak, readonly) FIRCLSFABNetworkClient *networkClient;
@property(nonatomic, readonly) NSDictionary *kitVersionsByKitBundleIdentifier;
@property(nonatomic, readonly) FIRCLSSettings *settings;
@end

@implementation FIRCLSOnboardingOperation

#pragma mark lifecycle methods

- (instancetype)initWithDelegate:(id<FIRCLSOnboardingOperationDelegate>)delegate
                        shouldCreate:(BOOL)shouldCreate
                         googleAppID:(NSString *)googleAppID
    kitVersionsByKitBundleIdentifier:(NSDictionary *)kitVersionsByKitBundleIdentifier
                  appIdentifierModel:(FIRCLSApplicationIdentifierModel *)appIdentifierModel
                      endpointString:(NSString *)appEndPoint
                       networkClient:(FIRCLSFABNetworkClient *)networkClient
                               token:(FIRCLSDataCollectionToken *)token
                            settings:(FIRCLSSettings *)settings {
  NSParameterAssert(appIdentifierModel);
  NSParameterAssert(appEndPoint);

  self = [super initWithGoogleAppID:googleAppID token:token];
  if (self) {
    _shouldCreate = shouldCreate;
    _delegate = delegate;
    _appIdentifierModel = appIdentifierModel;
    _appEndpoint = appEndPoint;
    _networkClient = networkClient;
    _kitVersionsByKitBundleIdentifier = kitVersionsByKitBundleIdentifier.copy;
    _settings = settings;
  }
  return self;
}

- (void)main {
  [self beginAppConfigure];
}

- (void)beginAppConfigure {
  NSOutputStream *stream = [[NSOutputStream alloc] initToMemory];
  NSString *boundary = [FIRCLSMultipartMimeStreamEncoder generateBoundary];

  FIRCLSMultipartMimeStreamEncoder *encoder =
      [FIRCLSMultipartMimeStreamEncoder encoderWithStream:stream andBoundary:boundary];
  if (!encoder) {
    FIRCLSErrorLog(@"Configure failed during onboarding");
    [self finishWithError:[self errorForCode:FIRCLSOnboardingErrorMultipartMimeConfiguration
                                    userInfo:@{
                                      NSLocalizedDescriptionKey : @"Multipart mime encoder was nil"
                                    }]];
    return;
  }

  NSString *orgID = [self.settings orgID];
  if (!orgID) {
    FIRCLSErrorLog(@"Could not onboard app with missing Organization ID");
    [self finishWithError:[self errorForCode:FIRCLSOnboardingErrorMultipartMimeConfiguration
                                    userInfo:@{
                                      NSLocalizedDescriptionKey : @"Organization ID was nil"
                                    }]];
    return;
  }

  [encoder encode:^{
    [encoder addValue:orgID fieldName:@"org_id"];

    [encoder addValue:self.settings.fetchedBundleID fieldName:@"app[identifier]"];
    [encoder addValue:self.appIdentifierModel.buildInstanceID
            fieldName:@"app[instance_identifier]"];
    [encoder addValue:self.appIdentifierModel.displayName fieldName:@"app[name]"];
    [encoder addValue:self.appIdentifierModel.buildVersion fieldName:@"app[build_version]"];
    [encoder addValue:self.appIdentifierModel.displayVersion fieldName:@"app[display_version]"];
    [encoder addValue:@(self.appIdentifierModel.installSource) fieldName:@"app[source]"];
    [encoder addValue:self.appIdentifierModel.minimumSDKString
            fieldName:@"app[minimum_sdk_version]"];
    [encoder addValue:self.appIdentifierModel.builtSDKString fieldName:@"app[built_sdk_version]"];
    [self.kitVersionsByKitBundleIdentifier
        enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
          NSString *formKey = [NSString stringWithFormat:@"%@[%@]", @"app[build][libraries]", key];
          [encoder addValue:obj fieldName:formKey];
        }];

    [self.appIdentifierModel.architectureUUIDMap
        enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL *stop) {
          [encoder addValue:key fieldName:@"app[slices][][arch]"];
          [encoder addValue:obj fieldName:@"app[slices][][uuid]"];
        }];
  }];

  NSMutableURLRequest *request = [self onboardingRequestForAppCreate:self.shouldCreate];
  [request setValue:orgID forHTTPHeaderField:FIRCLSNetworkCrashlyticsOrgId];

  [request setValue:encoder.contentTypeHTTPHeaderValue forHTTPHeaderField:@"Content-Type"];
  [request setValue:encoder.contentLengthHTTPHeaderValue forHTTPHeaderField:@"Content-Length"];
  [request setHTTPBody:[stream propertyForKey:NSStreamDataWrittenToMemoryStreamKey]];

  // Retry only when onboarding an app for the first time, otherwise it'll overwhelm our servers
  NSUInteger retryLimit = self.shouldCreate ? 10 : 1;

  [self.networkClient
      startDataTaskWithRequest:request
                    retryLimit:retryLimit
             completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
               self->_error = error;
               if (!self.shouldCreate) {
                 [self.delegate onboardingOperation:self didCompleteAppUpdateWithError:error];
               } else {
                 [self.delegate onboardingOperation:self didCompleteAppCreationWithError:error];
               }
               [self finishWithError:error];
             }];
}

#pragma mark private methods

- (NSError *)errorForCode:(NSUInteger)code userInfo:(NSDictionary *)userInfo {
  return [NSError errorWithDomain:FIRCLSOnboardingErrorDomain code:code userInfo:userInfo];
}

- (NSURL *)appCreateURL {
  // https://update.crashlytics.com/spi/v1/platforms/mac/apps/com.crashlytics.mac

  FIRCLSURLBuilder *url = [FIRCLSURLBuilder URLWithBase:self.appEndpoint];

  [url appendComponent:@"/spi/v1/platforms/"];
  [url escapeAndAppendComponent:self.appIdentifierModel.platform];
  [url appendComponent:@"/apps"];

  return url.URL;
}

- (NSURL *)appUpdateURL {
  // https://update.crashlytics.com/spi/v1/platforms/mac/apps/com.crashlytics.mac

  FIRCLSURLBuilder *url = [FIRCLSURLBuilder URLWithBase:[self appEndpoint]];

  [url appendComponent:@"/spi/v1/platforms/"];
  [url escapeAndAppendComponent:self.appIdentifierModel.platform];
  [url appendComponent:@"/apps/"];
  [url escapeAndAppendComponent:self.settings.fetchedBundleID];

  return url.URL;
}

- (NSMutableURLRequest *)onboardingRequestForAppCreate:(BOOL)shouldCreate {
  const NSTimeInterval timeout = 10.0;
  NSURL *url = nil;
  NSString *httpVerb = nil;
  if (shouldCreate) {
    httpVerb = @"POST";
    url = self.appCreateURL;
  } else {
    httpVerb = @"PUT";
    url = self.appUpdateURL;
  }
  NSMutableURLRequest *request = [self mutableRequestWithDefaultHTTPHeadersForURL:url
                                                                          timeout:timeout];
  request.HTTPMethod = httpVerb;
  return request;
}

@end
