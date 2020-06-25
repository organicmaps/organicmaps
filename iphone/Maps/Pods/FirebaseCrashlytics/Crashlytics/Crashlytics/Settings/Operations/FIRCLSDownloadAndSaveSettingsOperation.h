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
#import "FIRCLSNetworkOperation.h"

@class FIRCLSDownloadAndSaveSettingsOperation;
@class FIRCLSFABNetworkClient;
@class FIRCLSInstallIdentifierModel;

NS_ASSUME_NONNULL_BEGIN

/**
 * This is the protocol that a delegate of FIRCLSDownloadAndSaveSettingsOperation needs to follow.
 */
@protocol FIRCLSDownloadAndSaveSettingsOperationDelegate <NSObject>

@required

/**
 * Method that is called when settings have been downloaded and saved, or an error has occurred
 * during the operation. This method may be called on an arbitrary background thread.
 */
- (void)operation:(FIRCLSDownloadAndSaveSettingsOperation *)operation
    didDownloadAndSaveSettingsWithError:(nullable NSError *)error;

@end

/**
 * This operation downloads settings from the backend servers, and saves them in file on disk.
 */
@interface FIRCLSDownloadAndSaveSettingsOperation : FIRCLSNetworkOperation

- (instancetype)initWithGoogleAppID:(NSString *)googleAppID
                              token:(FIRCLSDataCollectionToken *)token NS_UNAVAILABLE;

/**
 * @param googleAppID must NOT be nil
 * @param delegate gets a callback after settings have been downloaded or an error occurs.
 * @param settingsURL must NOT be nil. This is the URL to which a download request is made.
 * @param settingsDirectoryPath must NOT be nil. This is the directory on disk where the settings
 * are persisted.
 * @param settingsFilePath must NOT be nil. It is the full file path(including file name) in which
 * settings will be persisted on disk.
 * @param installIDModel must NOT be nil. This value is sent back to the backend to uniquely
 * identify the app install.
 */
- (instancetype)initWithGoogleAppID:(NSString *)googleAppID
                           delegate:(id<FIRCLSDownloadAndSaveSettingsOperationDelegate>)delegate
                        settingsURL:(NSURL *)settingsURL
              settingsDirectoryPath:(NSString *)settingsDirectoryPath
                   settingsFilePath:(NSString *)settingsFilePath
                     installIDModel:(FIRCLSInstallIdentifierModel *)installIDModel
                      networkClient:(FIRCLSFABNetworkClient *)networkClient
                              token:(FIRCLSDataCollectionToken *)token NS_DESIGNATED_INITIALIZER;

/**
 * Delegate of this operation.
 */
@property(nonatomic, readonly, weak) id<FIRCLSDownloadAndSaveSettingsOperationDelegate> delegate;

/**
 * When an error occurs during this operation, it is made available in this property.
 */
@property(nonatomic, readonly) NSError *error;

@end

NS_ASSUME_NONNULL_END
