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

#import "FIRInstallationsItem.h"

#import "FIRInstallationsStoredAuthToken.h"
#import "FIRInstallationsStoredItem.h"

@implementation FIRInstallationsItem

- (instancetype)initWithAppID:(NSString *)appID firebaseAppName:(NSString *)firebaseAppName {
  self = [super init];
  if (self) {
    _appID = [appID copy];
    _firebaseAppName = [firebaseAppName copy];
  }
  return self;
}

- (nonnull id)copyWithZone:(nullable NSZone *)zone {
  FIRInstallationsItem *clone = [[FIRInstallationsItem alloc] initWithAppID:self.appID
                                                            firebaseAppName:self.firebaseAppName];
  clone.firebaseInstallationID = [self.firebaseInstallationID copy];
  clone.refreshToken = [self.refreshToken copy];
  clone.authToken = [self.authToken copy];
  clone.registrationStatus = self.registrationStatus;

  return clone;
}

- (void)updateWithStoredItem:(FIRInstallationsStoredItem *)item {
  self.firebaseInstallationID = item.firebaseInstallationID;
  self.refreshToken = item.refreshToken;
  self.authToken = item.authToken;
  self.registrationStatus = item.registrationStatus;
  self.IIDDefaultToken = item.IIDDefaultToken;
}

- (FIRInstallationsStoredItem *)storedItem {
  FIRInstallationsStoredItem *storedItem = [[FIRInstallationsStoredItem alloc] init];
  storedItem.firebaseInstallationID = self.firebaseInstallationID;
  storedItem.refreshToken = self.refreshToken;
  storedItem.authToken = self.authToken;
  storedItem.registrationStatus = self.registrationStatus;
  storedItem.IIDDefaultToken = self.IIDDefaultToken;
  return storedItem;
}

- (nonnull NSString *)identifier {
  return [[self class] identifierWithAppID:self.appID appName:self.firebaseAppName];
}

+ (NSString *)identifierWithAppID:(NSString *)appID appName:(NSString *)appName {
  return [appID stringByAppendingString:appName];
}

+ (NSString *)generateFID {
  NSUUID *UUID = [NSUUID UUID];
  uuid_t UUIDBytes;
  [UUID getUUIDBytes:UUIDBytes];

  NSUInteger UUIDLength = sizeof(uuid_t);
  NSData *UUIDData = [NSData dataWithBytes:UUIDBytes length:UUIDLength];

  uint8_t UUIDLast4Bits = UUIDBytes[UUIDLength - 1] & 0b00001111;

  // FID first 4 bits must be `0111`. The last 4 UUID bits will be cut later to form a proper FID.
  // To keep 16 random bytes we copy these last 4 UUID to the FID 1st byte after `0111` prefix.
  uint8_t FIDPrefix = 0b01110000 | UUIDLast4Bits;
  NSMutableData *FIDData = [NSMutableData dataWithBytes:&FIDPrefix length:1];

  [FIDData appendData:UUIDData];
  NSString *FIDString = [self base64URLEncodedStringWithData:FIDData];

  // A valid FID has exactly 22 base64 characters, which is 132 bits, or 16.5 bytes.
  // Our generated ID has 16 bytes UUID + 1 byte prefix which after encoding with base64 will become
  // 23 characters plus 1 character for "=" padding.

  // Remove the 23rd character that was added because of the extra 4 bits at the
  // end of our 17 byte data and the '=' padding.
  return [FIDString substringWithRange:NSMakeRange(0, 22)];
}

+ (NSString *)base64URLEncodedStringWithData:(NSData *)data {
  NSString *string = [data base64EncodedStringWithOptions:0];
  string = [string stringByReplacingOccurrencesOfString:@"/" withString:@"_"];
  string = [string stringByReplacingOccurrencesOfString:@"+" withString:@"-"];
  return string;
}

@end
