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

#import "FIRInstallationsStoredItem.h"

#import "FIRInstallationsLogger.h"
#import "FIRInstallationsStoredAuthToken.h"

NSString *const kFIRInstallationsStoredItemFirebaseInstallationIDKey = @"firebaseInstallationID";
NSString *const kFIRInstallationsStoredItemRefreshTokenKey = @"refreshToken";
NSString *const kFIRInstallationsStoredItemAuthTokenKey = @"authToken";
NSString *const kFIRInstallationsStoredItemRegistrationStatusKey = @"registrationStatus";
NSString *const kFIRInstallationsStoredItemIIDDefaultTokenKey = @"IIDDefaultToken";
NSString *const kFIRInstallationsStoredItemStorageVersionKey = @"storageVersion";

NSInteger const kFIRInstallationsStoredItemStorageVersion = 1;

@implementation FIRInstallationsStoredItem

- (NSInteger)storageVersion {
  return kFIRInstallationsStoredItemStorageVersion;
}

- (void)encodeWithCoder:(nonnull NSCoder *)aCoder {
  [aCoder encodeObject:self.firebaseInstallationID
                forKey:kFIRInstallationsStoredItemFirebaseInstallationIDKey];
  [aCoder encodeObject:self.refreshToken forKey:kFIRInstallationsStoredItemRefreshTokenKey];
  [aCoder encodeObject:self.authToken forKey:kFIRInstallationsStoredItemAuthTokenKey];
  [aCoder encodeInteger:self.registrationStatus
                 forKey:kFIRInstallationsStoredItemRegistrationStatusKey];
  [aCoder encodeObject:self.IIDDefaultToken forKey:kFIRInstallationsStoredItemIIDDefaultTokenKey];
  [aCoder encodeInteger:self.storageVersion forKey:kFIRInstallationsStoredItemStorageVersionKey];
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)aDecoder {
  NSInteger storageVersion =
      [aDecoder decodeIntegerForKey:kFIRInstallationsStoredItemStorageVersionKey];
  if (storageVersion > self.storageVersion) {
    FIRLogWarning(kFIRLoggerInstallations,
                  kFIRInstallationsMessageCodeInstallationCoderVersionMismatch,
                  @"FIRInstallationsStoredItem was encoded by a newer coder version %ld. Current "
                  @"coder version is %ld. Some installation data may be lost.",
                  (long)storageVersion, (long)kFIRInstallationsStoredItemStorageVersion);
  }

  FIRInstallationsStoredItem *item = [[FIRInstallationsStoredItem alloc] init];
  item.firebaseInstallationID =
      [aDecoder decodeObjectOfClass:[NSString class]
                             forKey:kFIRInstallationsStoredItemFirebaseInstallationIDKey];
  item.refreshToken = [aDecoder decodeObjectOfClass:[NSString class]
                                             forKey:kFIRInstallationsStoredItemRefreshTokenKey];
  item.authToken = [aDecoder decodeObjectOfClass:[FIRInstallationsStoredAuthToken class]
                                          forKey:kFIRInstallationsStoredItemAuthTokenKey];
  item.registrationStatus =
      [aDecoder decodeIntegerForKey:kFIRInstallationsStoredItemRegistrationStatusKey];
  item.IIDDefaultToken =
      [aDecoder decodeObjectOfClass:[NSString class]
                             forKey:kFIRInstallationsStoredItemIIDDefaultTokenKey];

  return item;
}

+ (BOOL)supportsSecureCoding {
  return YES;
}

@end
