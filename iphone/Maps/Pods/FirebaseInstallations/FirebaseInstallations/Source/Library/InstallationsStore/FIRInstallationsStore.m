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

#import "FIRInstallationsStore.h"

#import <GoogleUtilities/GULUserDefaults.h>

#if __has_include(<FBLPromises/FBLPromises.h>)
#import <FBLPromises/FBLPromises.h>
#else
#import "FBLPromises.h"
#endif

#import <GoogleUtilities/GULKeychainStorage.h>

#import "FIRInstallationsErrorUtil.h"
#import "FIRInstallationsItem.h"
#import "FIRInstallationsStoredItem.h"

NSString *const kFIRInstallationsStoreUserDefaultsID = @"com.firebase.FIRInstallations";

@interface FIRInstallationsStore ()
@property(nonatomic, readonly) GULKeychainStorage *secureStorage;
@property(nonatomic, readonly, nullable) NSString *accessGroup;
@property(nonatomic, readonly) dispatch_queue_t queue;
@property(nonatomic, readonly) GULUserDefaults *userDefaults;
@end

@implementation FIRInstallationsStore

- (instancetype)initWithSecureStorage:(GULKeychainStorage *)storage
                          accessGroup:(NSString *)accessGroup {
  self = [super init];
  if (self) {
    _secureStorage = storage;
    _accessGroup = [accessGroup copy];
    _queue = dispatch_queue_create("com.firebase.FIRInstallationsStore", DISPATCH_QUEUE_SERIAL);

    NSString *userDefaultsSuiteName = _accessGroup ?: kFIRInstallationsStoreUserDefaultsID;
    _userDefaults = [[GULUserDefaults alloc] initWithSuiteName:userDefaultsSuiteName];
  }
  return self;
}

- (FBLPromise<FIRInstallationsItem *> *)installationForAppID:(NSString *)appID
                                                     appName:(NSString *)appName {
  NSString *itemID = [FIRInstallationsItem identifierWithAppID:appID appName:appName];
  return [self installationExistsForAppID:appID appName:appName]
      .then(^id(id result) {
        return [self.secureStorage getObjectForKey:itemID
                                       objectClass:[FIRInstallationsStoredItem class]
                                       accessGroup:self.accessGroup];
      })
      .then(^id(FIRInstallationsStoredItem *_Nullable storedItem) {
        if (storedItem == nil) {
          return [FIRInstallationsErrorUtil installationItemNotFoundForAppID:appID appName:appName];
        }

        FIRInstallationsItem *item = [[FIRInstallationsItem alloc] initWithAppID:appID
                                                                 firebaseAppName:appName];
        [item updateWithStoredItem:storedItem];
        return item;
      });
}

- (FBLPromise<NSNull *> *)saveInstallation:(FIRInstallationsItem *)installationItem {
  FIRInstallationsStoredItem *storedItem = [installationItem storedItem];
  NSString *identifier = [installationItem identifier];

  return
      [self.secureStorage setObject:storedItem forKey:identifier accessGroup:self.accessGroup].then(
          ^id(id result) {
            return [self setInstallationExists:YES forItemWithIdentifier:identifier];
          });
}

- (FBLPromise<NSNull *> *)removeInstallationForAppID:(NSString *)appID appName:(NSString *)appName {
  NSString *identifier = [FIRInstallationsItem identifierWithAppID:appID appName:appName];
  return [self.secureStorage removeObjectForKey:identifier accessGroup:self.accessGroup].then(
      ^id(id result) {
        return [self setInstallationExists:NO forItemWithIdentifier:identifier];
      });
}

#pragma mark - User defaults

- (FBLPromise<NSNull *> *)installationExistsForAppID:(NSString *)appID appName:(NSString *)appName {
  NSString *identifier = [FIRInstallationsItem identifierWithAppID:appID appName:appName];
  return [FBLPromise onQueue:self.queue
                          do:^id _Nullable {
                            return [[self userDefaults] objectForKey:identifier] != nil
                                       ? [NSNull null]
                                       : [FIRInstallationsErrorUtil
                                             installationItemNotFoundForAppID:appID
                                                                      appName:appName];
                          }];
}

- (FBLPromise<NSNull *> *)setInstallationExists:(BOOL)exists
                          forItemWithIdentifier:(NSString *)identifier {
  return [FBLPromise onQueue:self.queue
                          do:^id _Nullable {
                            if (exists) {
                              [[self userDefaults] setBool:YES forKey:identifier];
                            } else {
                              [[self userDefaults] removeObjectForKey:identifier];
                            }

                            return [NSNull null];
                          }];
}

@end
