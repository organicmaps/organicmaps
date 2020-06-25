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

#import "FIRInstallationsStoredAuthToken.h"

#import "FIRInstallationsLogger.h"

NSString *const kFIRInstallationsStoredAuthTokenStatusKey = @"status";
NSString *const kFIRInstallationsStoredAuthTokenTokenKey = @"token";
NSString *const kFIRInstallationsStoredAuthTokenExpirationDateKey = @"expirationDate";
NSString *const kFIRInstallationsStoredAuthTokenStorageVersionKey = @"storageVersion";

NSInteger const kFIRInstallationsStoredAuthTokenStorageVersion = 1;

@implementation FIRInstallationsStoredAuthToken

- (NSInteger)storageVersion {
  return kFIRInstallationsStoredAuthTokenStorageVersion;
}

- (nonnull id)copyWithZone:(nullable NSZone *)zone {
  FIRInstallationsStoredAuthToken *clone = [[FIRInstallationsStoredAuthToken alloc] init];
  clone.status = self.status;
  clone.token = [self.token copy];
  clone.expirationDate = self.expirationDate;
  return clone;
}

- (void)encodeWithCoder:(nonnull NSCoder *)aCoder {
  [aCoder encodeInteger:self.status forKey:kFIRInstallationsStoredAuthTokenStatusKey];
  [aCoder encodeObject:self.token forKey:kFIRInstallationsStoredAuthTokenTokenKey];
  [aCoder encodeObject:self.expirationDate
                forKey:kFIRInstallationsStoredAuthTokenExpirationDateKey];
  [aCoder encodeInteger:self.storageVersion
                 forKey:kFIRInstallationsStoredAuthTokenStorageVersionKey];
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)aDecoder {
  NSInteger storageVersion =
      [aDecoder decodeIntegerForKey:kFIRInstallationsStoredAuthTokenStorageVersionKey];
  if (storageVersion > kFIRInstallationsStoredAuthTokenStorageVersion) {
    FIRLogWarning(kFIRLoggerInstallations,
                  kFIRInstallationsMessageCodeAuthTokenCoderVersionMismatch,
                  @"FIRInstallationsStoredAuthToken was encoded by a newer coder version %ld. "
                  @"Current coder version is %ld. Some auth token data may be lost.",
                  (long)storageVersion, (long)kFIRInstallationsStoredAuthTokenStorageVersion);
  }

  FIRInstallationsStoredAuthToken *object = [[FIRInstallationsStoredAuthToken alloc] init];
  object.status = [aDecoder decodeIntegerForKey:kFIRInstallationsStoredAuthTokenStatusKey];
  object.token = [aDecoder decodeObjectOfClass:[NSString class]
                                        forKey:kFIRInstallationsStoredAuthTokenTokenKey];
  object.expirationDate =
      [aDecoder decodeObjectOfClass:[NSDate class]
                             forKey:kFIRInstallationsStoredAuthTokenExpirationDateKey];

  return object;
}

+ (BOOL)supportsSecureCoding {
  return YES;
}

@end
