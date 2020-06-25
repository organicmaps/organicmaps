/*
 * Copyright 2019 Google
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

#import "FIRInstallationsIDController.h"

#if __has_include(<FBLPromises/FBLPromises.h>)
#import <FBLPromises/FBLPromises.h>
#else
#import "FBLPromises.h"
#endif

#import <FirebaseCore/FIRAppInternal.h>
#import <GoogleUtilities/GULKeychainStorage.h>

#import "FIRInstallationsAPIService.h"
#import "FIRInstallationsErrorUtil.h"
#import "FIRInstallationsIIDStore.h"
#import "FIRInstallationsIIDTokenStore.h"
#import "FIRInstallationsItem.h"
#import "FIRInstallationsLogger.h"
#import "FIRInstallationsSingleOperationPromiseCache.h"
#import "FIRInstallationsStore.h"

#import "FIRInstallationsHTTPError.h"
#import "FIRInstallationsStoredAuthToken.h"

const NSNotificationName FIRInstallationIDDidChangeNotification =
    @"FIRInstallationIDDidChangeNotification";
NSString *const kFIRInstallationIDDidChangeNotificationAppNameKey =
    @"FIRInstallationIDDidChangeNotification";

NSTimeInterval const kFIRInstallationsTokenExpirationThreshold = 60 * 60;  // 1 hour.

static NSString *const kKeychainService = @"com.firebase.FIRInstallations.installations";

@interface FIRInstallationsIDController ()
@property(nonatomic, readonly) NSString *appID;
@property(nonatomic, readonly) NSString *appName;

@property(nonatomic, readonly) FIRInstallationsStore *installationsStore;
@property(nonatomic, readonly) FIRInstallationsIIDStore *IIDStore;
@property(nonatomic, readonly) FIRInstallationsIIDTokenStore *IIDTokenStore;

@property(nonatomic, readonly) FIRInstallationsAPIService *APIService;

@property(nonatomic, readonly) FIRInstallationsSingleOperationPromiseCache<FIRInstallationsItem *>
    *getInstallationPromiseCache;
@property(nonatomic, readonly)
    FIRInstallationsSingleOperationPromiseCache<FIRInstallationsItem *> *authTokenPromiseCache;
@property(nonatomic, readonly) FIRInstallationsSingleOperationPromiseCache<FIRInstallationsItem *>
    *authTokenForcingRefreshPromiseCache;
@property(nonatomic, readonly)
    FIRInstallationsSingleOperationPromiseCache<NSNull *> *deleteInstallationPromiseCache;
@end

@implementation FIRInstallationsIDController

- (instancetype)initWithGoogleAppID:(NSString *)appID
                            appName:(NSString *)appName
                             APIKey:(NSString *)APIKey
                          projectID:(NSString *)projectID
                        GCMSenderID:(NSString *)GCMSenderID
                        accessGroup:(nullable NSString *)accessGroup {
  NSString *serviceName = [FIRInstallationsIDController keychainServiceWithAppID:appID];
  GULKeychainStorage *secureStorage = [[GULKeychainStorage alloc] initWithService:serviceName];
  FIRInstallationsStore *installationsStore =
      [[FIRInstallationsStore alloc] initWithSecureStorage:secureStorage accessGroup:accessGroup];

  // Use `GCMSenderID` as project identifier when `projectID` is not available.
  NSString *APIServiceProjectID = (projectID.length > 0) ? projectID : GCMSenderID;
  FIRInstallationsAPIService *apiService =
      [[FIRInstallationsAPIService alloc] initWithAPIKey:APIKey projectID:APIServiceProjectID];

  FIRInstallationsIIDStore *IIDStore = [[FIRInstallationsIIDStore alloc] init];
  FIRInstallationsIIDTokenStore *IIDCheckingStore =
      [[FIRInstallationsIIDTokenStore alloc] initWithGCMSenderID:GCMSenderID];

  return [self initWithGoogleAppID:appID
                           appName:appName
                installationsStore:installationsStore
                        APIService:apiService
                          IIDStore:IIDStore
                     IIDTokenStore:IIDCheckingStore];
}

/// The initializer is supposed to be used by tests to inject `installationsStore`.
- (instancetype)initWithGoogleAppID:(NSString *)appID
                            appName:(NSString *)appName
                 installationsStore:(FIRInstallationsStore *)installationsStore
                         APIService:(FIRInstallationsAPIService *)APIService
                           IIDStore:(FIRInstallationsIIDStore *)IIDStore
                      IIDTokenStore:(FIRInstallationsIIDTokenStore *)IIDTokenStore {
  self = [super init];
  if (self) {
    _appID = appID;
    _appName = appName;
    _installationsStore = installationsStore;
    _APIService = APIService;
    _IIDStore = IIDStore;
    _IIDTokenStore = IIDTokenStore;

    __weak FIRInstallationsIDController *weakSelf = self;

    _getInstallationPromiseCache = [[FIRInstallationsSingleOperationPromiseCache alloc]
        initWithNewOperationHandler:^FBLPromise *_Nonnull {
          FIRInstallationsIDController *strongSelf = weakSelf;
          return [strongSelf createGetInstallationItemPromise];
        }];

    _authTokenPromiseCache = [[FIRInstallationsSingleOperationPromiseCache alloc]
        initWithNewOperationHandler:^FBLPromise *_Nonnull {
          FIRInstallationsIDController *strongSelf = weakSelf;
          return [strongSelf installationWithValidAuthTokenForcingRefresh:NO];
        }];

    _authTokenForcingRefreshPromiseCache = [[FIRInstallationsSingleOperationPromiseCache alloc]
        initWithNewOperationHandler:^FBLPromise *_Nonnull {
          FIRInstallationsIDController *strongSelf = weakSelf;
          return [strongSelf installationWithValidAuthTokenForcingRefresh:YES];
        }];

    _deleteInstallationPromiseCache = [[FIRInstallationsSingleOperationPromiseCache alloc]
        initWithNewOperationHandler:^FBLPromise *_Nonnull {
          FIRInstallationsIDController *strongSelf = weakSelf;
          return [strongSelf createDeleteInstallationPromise];
        }];
  }
  return self;
}

#pragma mark - Get Installation.

- (FBLPromise<FIRInstallationsItem *> *)getInstallationItem {
  return [self.getInstallationPromiseCache getExistingPendingOrCreateNewPromise];
}

- (FBLPromise<FIRInstallationsItem *> *)createGetInstallationItemPromise {
  FIRLogDebug(kFIRLoggerInstallations,
              kFIRInstallationsMessageCodeNewGetInstallationOperationCreated, @"%s, appName: %@",
              __PRETTY_FUNCTION__, self.appName);

  FBLPromise<FIRInstallationsItem *> *installationItemPromise =
      [self getStoredInstallation].recover(^id(NSError *error) {
        return [self createAndSaveFID];
      });

  // Initiate registration process on success if needed, but return the installation without waiting
  // for it.
  installationItemPromise.then(^id(FIRInstallationsItem *installation) {
    [self getAuthTokenForcingRefresh:NO];
    return nil;
  });

  return installationItemPromise;
}

- (FBLPromise<FIRInstallationsItem *> *)getStoredInstallation {
  return [self.installationsStore installationForAppID:self.appID appName:self.appName].validate(
      ^BOOL(FIRInstallationsItem *installation) {
        BOOL isValid = NO;
        switch (installation.registrationStatus) {
          case FIRInstallationStatusUnregistered:
          case FIRInstallationStatusRegistered:
            isValid = YES;
            break;

          case FIRInstallationStatusUnknown:
            isValid = NO;
            break;
        }

        return isValid;
      });
}

- (FBLPromise<FIRInstallationsItem *> *)createAndSaveFID {
  return [self migrateOrGenerateInstallation]
      .then(^FBLPromise<FIRInstallationsItem *> *(FIRInstallationsItem *installation) {
        return [self saveInstallation:installation];
      })
      .then(^FIRInstallationsItem *(FIRInstallationsItem *installation) {
        [self postFIDDidChangeNotification];
        return installation;
      });
}

- (FBLPromise<FIRInstallationsItem *> *)saveInstallation:(FIRInstallationsItem *)installation {
  return [self.installationsStore saveInstallation:installation].then(
      ^FIRInstallationsItem *(NSNull *result) {
        return installation;
      });
}

/**
 * Tries to migrate IID data stored by FirebaseInstanceID SDK or generates a new Installation ID if
 * not found.
 */
- (FBLPromise<FIRInstallationsItem *> *)migrateOrGenerateInstallation {
  if (![self isDefaultApp]) {
    // Existing IID should be used only for default FirebaseApp.
    FIRInstallationsItem *installation =
        [self createInstallationWithFID:[FIRInstallationsItem generateFID] IIDDefaultToken:nil];
    return [FBLPromise resolvedWith:installation];
  }

  return [[[FBLPromise
      all:@[ [self.IIDStore existingIID], [self.IIDTokenStore existingIIDDefaultToken] ]]
      then:^id _Nullable(NSArray *_Nullable results) {
        NSString *existingIID = results[0];
        NSString *IIDDefaultToken = results[1];

        return [self createInstallationWithFID:existingIID IIDDefaultToken:IIDDefaultToken];
      }] recover:^id _Nullable(NSError *_Nonnull error) {
    return [self createInstallationWithFID:[FIRInstallationsItem generateFID] IIDDefaultToken:nil];
  }];
}

- (FIRInstallationsItem *)createInstallationWithFID:(NSString *)FID
                                    IIDDefaultToken:(nullable NSString *)IIDDefaultToken {
  FIRInstallationsItem *installation = [[FIRInstallationsItem alloc] initWithAppID:self.appID
                                                                   firebaseAppName:self.appName];
  installation.firebaseInstallationID = FID;
  installation.IIDDefaultToken = IIDDefaultToken;
  installation.registrationStatus = FIRInstallationStatusUnregistered;
  return installation;
}

#pragma mark - FID registration

- (FBLPromise<FIRInstallationsItem *> *)registerInstallationIfNeeded:
    (FIRInstallationsItem *)installation {
  switch (installation.registrationStatus) {
    case FIRInstallationStatusRegistered:
      // Already registered. Do nothing.
      return [FBLPromise resolvedWith:installation];

    case FIRInstallationStatusUnknown:
    case FIRInstallationStatusUnregistered:
      // Registration required. Proceed.
      break;
  }

  return [self.APIService registerInstallation:installation]
      .catch(^(NSError *_Nonnull error) {
        if ([self doesRegistrationErrorRequireConfigChange:error]) {
          FIRLogError(kFIRLoggerInstallations,
                      kFIRInstallationsMessageCodeInvalidFirebaseConfiguration,
                      @"Firebase Installation registration failed for app with name: %@, error:\n"
                      @"%@\nPlease make sure you use valid GoogleService-Info.plist",
                      self.appName, error.userInfo[NSLocalizedFailureReasonErrorKey]);
        }
      })
      .then(^id(FIRInstallationsItem *registeredInstallation) {
        return [self saveInstallation:registeredInstallation];
      })
      .then(^FIRInstallationsItem *(FIRInstallationsItem *registeredInstallation) {
        // Server may respond with a different FID if the sent one cannot be accepted.
        if (![registeredInstallation.firebaseInstallationID
                isEqualToString:installation.firebaseInstallationID]) {
          [self postFIDDidChangeNotification];
        }
        return registeredInstallation;
      });
}

- (BOOL)doesRegistrationErrorRequireConfigChange:(NSError *)error {
  FIRInstallationsHTTPError *HTTPError = (FIRInstallationsHTTPError *)error;
  if (![HTTPError isKindOfClass:[FIRInstallationsHTTPError class]]) {
    return NO;
  }

  switch (HTTPError.HTTPResponse.statusCode) {
    // These are the errors that require Firebase configuration change.
    case FIRInstallationsRegistrationHTTPCodeInvalidArgument:
    case FIRInstallationsRegistrationHTTPCodeInvalidAPIKey:
    case FIRInstallationsRegistrationHTTPCodeAPIKeyToProjectIDMismatch:
    case FIRInstallationsRegistrationHTTPCodeProjectNotFound:
      return YES;

    default:
      return NO;
  }
}

#pragma mark - Auth Token

- (FBLPromise<FIRInstallationsItem *> *)getAuthTokenForcingRefresh:(BOOL)forceRefresh {
  if (forceRefresh || [self.authTokenForcingRefreshPromiseCache getExistingPendingPromise] != nil) {
    return [self.authTokenForcingRefreshPromiseCache getExistingPendingOrCreateNewPromise];
  } else {
    return [self.authTokenPromiseCache getExistingPendingOrCreateNewPromise];
  }
}

- (FBLPromise<FIRInstallationsItem *> *)installationWithValidAuthTokenForcingRefresh:
    (BOOL)forceRefresh {
  FIRLogDebug(kFIRLoggerInstallations, kFIRInstallationsMessageCodeNewGetAuthTokenOperationCreated,
              @"-[FIRInstallationsIDController installationWithValidAuthTokenForcingRefresh:%@], "
              @"appName: %@",
              @(forceRefresh), self.appName);

  return [self getInstallationItem]
      .then(^FBLPromise<FIRInstallationsItem *> *(FIRInstallationsItem *installation) {
        return [self registerInstallationIfNeeded:installation];
      })
      .then(^id(FIRInstallationsItem *registeredInstallation) {
        BOOL isTokenExpiredOrExpiresSoon =
            [registeredInstallation.authToken.expirationDate timeIntervalSinceDate:[NSDate date]] <
            kFIRInstallationsTokenExpirationThreshold;
        if (forceRefresh || isTokenExpiredOrExpiresSoon) {
          return [self refreshAuthTokenForInstallation:registeredInstallation];
        } else {
          return registeredInstallation;
        }
      })
      .recover(^id(NSError *error) {
        return [self regenerateFIDOnRefreshTokenErrorIfNeeded:error];
      });
}

- (FBLPromise<FIRInstallationsItem *> *)refreshAuthTokenForInstallation:
    (FIRInstallationsItem *)installation {
  return [[self.APIService refreshAuthTokenForInstallation:installation]
      then:^id _Nullable(FIRInstallationsItem *_Nullable refreshedInstallation) {
        return [self saveInstallation:refreshedInstallation];
      }];
}

- (id)regenerateFIDOnRefreshTokenErrorIfNeeded:(NSError *)error {
  if (![error isKindOfClass:[FIRInstallationsHTTPError class]]) {
    // No recovery possible. Return the same error.
    return error;
  }

  FIRInstallationsHTTPError *HTTPError = (FIRInstallationsHTTPError *)error;
  switch (HTTPError.HTTPResponse.statusCode) {
    case FIRInstallationsAuthTokenHTTPCodeInvalidAuthentication:
    case FIRInstallationsAuthTokenHTTPCodeFIDNotFound:
      // The stored installation was damaged or blocked by the server.
      // Delete the stored installation then generate and register a new one.
      return [self getInstallationItem]
          .then(^FBLPromise<NSNull *> *(FIRInstallationsItem *installation) {
            return [self deleteInstallationLocally:installation];
          })
          .then(^FBLPromise<FIRInstallationsItem *> *(id result) {
            return [self installationWithValidAuthTokenForcingRefresh:NO];
          });

    default:
      // No recovery possible. Return the same error.
      return error;
  }
}

#pragma mark - Delete FID

- (FBLPromise<NSNull *> *)deleteInstallation {
  return [self.deleteInstallationPromiseCache getExistingPendingOrCreateNewPromise];
}

- (FBLPromise<NSNull *> *)createDeleteInstallationPromise {
  FIRLogDebug(kFIRLoggerInstallations,
              kFIRInstallationsMessageCodeNewDeleteInstallationOperationCreated, @"%s, appName: %@",
              __PRETTY_FUNCTION__, self.appName);

  // Check for ongoing requests first, if there is no a request, then check local storage for
  // existing installation.
  FBLPromise<FIRInstallationsItem *> *currentInstallationPromise =
      [self mostRecentInstallationOperation] ?: [self getStoredInstallation];

  return currentInstallationPromise
      .then(^id(FIRInstallationsItem *installation) {
        return [self sendDeleteInstallationRequestIfNeeded:installation];
      })
      .then(^id(FIRInstallationsItem *installation) {
        // Remove the installation from the local storage.
        return [self deleteInstallationLocally:installation];
      });
}

- (FBLPromise<NSNull *> *)deleteInstallationLocally:(FIRInstallationsItem *)installation {
  return [self.installationsStore removeInstallationForAppID:installation.appID
                                                     appName:installation.firebaseAppName]
      .then(^FBLPromise<NSNull *> *(NSNull *result) {
        return [self deleteExistingIIDIfNeeded];
      })
      .then(^NSNull *(NSNull *result) {
        [self postFIDDidChangeNotification];
        return result;
      });
}

- (FBLPromise<FIRInstallationsItem *> *)sendDeleteInstallationRequestIfNeeded:
    (FIRInstallationsItem *)installation {
  switch (installation.registrationStatus) {
    case FIRInstallationStatusUnknown:
    case FIRInstallationStatusUnregistered:
      // The installation is not registered, so it is safe to be deleted as is, so return early.
      return [FBLPromise resolvedWith:installation];
      break;

    case FIRInstallationStatusRegistered:
      // Proceed to de-register the installation on the server.
      break;
  }

  return [self.APIService deleteInstallation:installation].recover(^id(NSError *APIError) {
    if ([FIRInstallationsErrorUtil isAPIError:APIError withHTTPCode:404]) {
      // The installation was not found on the server.
      // Return success.
      return installation;
    } else {
      // Re-throw the error otherwise.
      return APIError;
    }
  });
}

- (FBLPromise<NSNull *> *)deleteExistingIIDIfNeeded {
  if ([self isDefaultApp]) {
    return [self.IIDStore deleteExistingIID];
  } else {
    return [FBLPromise resolvedWith:[NSNull null]];
  }
}

- (nullable FBLPromise<FIRInstallationsItem *> *)mostRecentInstallationOperation {
  return [self.authTokenForcingRefreshPromiseCache getExistingPendingPromise]
             ?: [self.authTokenPromiseCache getExistingPendingPromise]
                    ?: [self.getInstallationPromiseCache getExistingPendingPromise];
}

#pragma mark - Notifications

- (void)postFIDDidChangeNotification {
  [[NSNotificationCenter defaultCenter]
      postNotificationName:FIRInstallationIDDidChangeNotification
                    object:nil
                  userInfo:@{kFIRInstallationIDDidChangeNotificationAppNameKey : self.appName}];
}

#pragma mark - Default App

- (BOOL)isDefaultApp {
  return [self.appName isEqualToString:kFIRDefaultAppName];
}

#pragma mark - Keychain

+ (NSString *)keychainServiceWithAppID:(NSString *)appID {
#if TARGET_OS_MACCATALYST || TARGET_OS_OSX
  // We need to keep service name unique per application on macOS.
  // Applications on macOS may request access to Keychain items stored by other applications. It
  // means that when the app looks up for a relevant Keychain item in the service scope it will
  // request user password to grant access to the Keychain if there are other Keychain items from
  // other applications stored under the same Keychain Service.
  return [kKeychainService stringByAppendingFormat:@".%@", appID];
#else
  // Use a constant Keychain service for non-macOS because:
  // 1. Keychain items cannot be shared between apps until configured specifically so the service
  // name collisions are not a concern
  // 2. We don't want to change the service name to avoid doing a migration.
  return kKeychainService;
#endif
}

@end
