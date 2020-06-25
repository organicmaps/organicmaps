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

#import "FIRCLSDownloadAndSaveSettingsOperation.h"

#import "FIRCLSConstants.h"
#import "FIRCLSFABHost.h"
#import "FIRCLSFABNetworkClient.h"
#import "FIRCLSInstallIdentifierModel.h"
#import "FIRCLSLogger.h"

@interface FIRCLSDownloadAndSaveSettingsOperation ()

/**
 * Method called to fetch the URL from where settings have to be downloaded.
 */
@property(readonly, nonatomic) NSURL *settingsURL;
/**
 * File manager which will be used to save settings on disk.
 */
@property(readonly, nonatomic) NSFileManager *fileManager;

/**
 * Directory path on which settings file will be saved
 */
@property(readonly, nonatomic) NSString *settingsDirectoryPath;
/**
 * Complete file path on which settings file will be saved
 */
@property(readonly, nonatomic) NSString *settingsFilePath;
/**
 * App install identifier.
 */
@property(strong, readonly, nonatomic) FIRCLSInstallIdentifierModel *installIDModel;

@property(weak, readonly, nonatomic) FIRCLSFABNetworkClient *networkClient;

@end

@implementation FIRCLSDownloadAndSaveSettingsOperation

- (instancetype)initWithGoogleAppID:(NSString *)googleAppID
                           delegate:(id<FIRCLSDownloadAndSaveSettingsOperationDelegate>)delegate
                        settingsURL:(NSURL *)settingsURL
              settingsDirectoryPath:(NSString *)settingsDirectoryPath
                   settingsFilePath:(NSString *)settingsFilePath
                     installIDModel:(FIRCLSInstallIdentifierModel *)installIDModel
                      networkClient:(FIRCLSFABNetworkClient *)networkClient
                              token:(FIRCLSDataCollectionToken *)token {
  NSParameterAssert(settingsURL);
  NSParameterAssert(settingsDirectoryPath);
  NSParameterAssert(settingsFilePath);
  NSParameterAssert(installIDModel);

  self = [super initWithGoogleAppID:googleAppID token:token];
  if (self) {
    _delegate = delegate;
    _settingsURL = settingsURL.copy;
    _settingsDirectoryPath = settingsDirectoryPath.copy;
    _settingsFilePath = settingsFilePath.copy;
    _fileManager = [[NSFileManager alloc] init];
    _installIDModel = installIDModel;
    _networkClient = networkClient;
  }
  return self;
}

- (NSMutableURLRequest *)mutableRequestWithDefaultHTTPHeaderFieldsAndTimeoutForURL:(NSURL *)url {
  NSMutableURLRequest *request =
      [super mutableRequestWithDefaultHTTPHeaderFieldsAndTimeoutForURL:url];
  request.HTTPMethod = @"GET";
  [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
  [request setValue:self.installIDModel.installID
      forHTTPHeaderField:@"X-Crashlytics-Installation-ID"];
  [request setValue:FIRCLSHostModelInfo() forHTTPHeaderField:@"X-Crashlytics-Device-Model"];
  [request setValue:FIRCLSHostOSBuildVersion()
      forHTTPHeaderField:@"X-Crashlytics-OS-Build-Version"];
  [request setValue:FIRCLSHostOSDisplayVersion()
      forHTTPHeaderField:@"X-Crashlytics-OS-Display-Version"];
  [request setValue:FIRCLSVersion forHTTPHeaderField:@"X-Crashlytics-API-Client-Version"];

  return request;
}

- (void)main {
  NSMutableURLRequest *request =
      [self mutableRequestWithDefaultHTTPHeaderFieldsAndTimeoutForURL:self.settingsURL];

  [self.networkClient
      startDownloadTaskWithRequest:request
                        retryLimit:1
                 completionHandler:^(NSURL *location, NSURLResponse *response, NSError *error) {
                   if (error) {
                     self->_error = error;
                     [self.delegate operation:self didDownloadAndSaveSettingsWithError:self.error];
                     [self finishWithError:error];
                     return;
                   }
                   // This move needs to happen synchronously, because after this method completes,
                   // the file will not be available.
                   NSError *moveError = nil;

                   // this removal will frequently fail, and we don't want the warning
                   [self.fileManager removeItemAtPath:self.settingsDirectoryPath error:nil];

                   [self.fileManager createDirectoryAtPath:self.settingsDirectoryPath
                               withIntermediateDirectories:YES
                                                attributes:nil
                                                     error:nil];
                   if (![self.fileManager moveItemAtPath:location.path
                                                  toPath:self.settingsFilePath
                                                   error:&moveError]) {
                     FIRCLSErrorLog(@"Unable to complete settings download %@", moveError);
                     self->_error = moveError;
                   }
                   [self.delegate operation:self didDownloadAndSaveSettingsWithError:self.error];
                   [self finishWithError:self.error];
                 }];
}

@end
