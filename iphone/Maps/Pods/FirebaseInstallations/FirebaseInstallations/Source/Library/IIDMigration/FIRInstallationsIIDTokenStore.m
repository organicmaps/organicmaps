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

#import "FIRInstallationsIIDTokenStore.h"

#if __has_include(<FBLPromises/FBLPromises.h>)
#import <FBLPromises/FBLPromises.h>
#else
#import "FBLPromises.h"
#endif

#import <GoogleUtilities/GULKeychainUtils.h>

#import "FIRInstallationsErrorUtil.h"

static NSString *const kFIRInstallationsIIDTokenKeychainId = @"com.google.iid-tokens";

@interface FIRInstallationsIIDTokenInfo : NSObject <NSSecureCoding>
@property(nonatomic, nullable, copy) NSString *token;
@end

@implementation FIRInstallationsIIDTokenInfo

+ (BOOL)supportsSecureCoding {
  return YES;
}

- (void)encodeWithCoder:(nonnull NSCoder *)coder {
}

- (nullable instancetype)initWithCoder:(nonnull NSCoder *)coder {
  self = [super init];
  if (self) {
    _token = [coder decodeObjectOfClass:[NSString class] forKey:@"token"];
  }
  return self;
}

@end

@interface FIRInstallationsIIDTokenStore ()
@property(nonatomic, readonly) NSString *GCMSenderID;
@end

@implementation FIRInstallationsIIDTokenStore

- (instancetype)initWithGCMSenderID:(NSString *)GCMSenderID {
  self = [super init];
  if (self) {
    _GCMSenderID = GCMSenderID;
  }
  return self;
}

- (FBLPromise<NSString *> *)existingIIDDefaultToken {
  return [[FBLPromise onQueue:dispatch_get_global_queue(QOS_CLASS_UTILITY, 0)
                           do:^id _Nullable {
                             return [self IIDDefaultTokenData];
                           }] onQueue:dispatch_get_global_queue(QOS_CLASS_UTILITY, 0)
                                 then:^id _Nullable(NSData *_Nullable keychainData) {
                                   return [self IIDCheckinWithData:keychainData];
                                 }];
}

- (FBLPromise<NSString *> *)IIDCheckinWithData:(NSData *)data {
  FBLPromise<NSString *> *resultPromise = [FBLPromise pendingPromise];

  NSError *archiverError;
  NSKeyedUnarchiver *unarchiver;
  if (@available(iOS 11.0, tvOS 11.0, macOS 10.13, *)) {
    unarchiver = [[NSKeyedUnarchiver alloc] initForReadingFromData:data error:&archiverError];
  } else {
    @try {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      unarchiver = [[NSKeyedUnarchiver alloc] initForReadingWithData:data];
#pragma clang diagnostic pop
    } @catch (NSException *exception) {
      archiverError = [FIRInstallationsErrorUtil keyedArchiverErrorWithException:exception];
    }
  }

  if (!unarchiver) {
    NSError *error = archiverError ?: [FIRInstallationsErrorUtil corruptedIIDTokenData];
    [resultPromise reject:error];
    return resultPromise;
  }

  [unarchiver setClass:[FIRInstallationsIIDTokenInfo class] forClassName:@"FIRInstanceIDTokenInfo"];
  FIRInstallationsIIDTokenInfo *IIDTokenInfo =
      [unarchiver decodeObjectOfClass:[FIRInstallationsIIDTokenInfo class]
                               forKey:NSKeyedArchiveRootObjectKey];

  if (IIDTokenInfo.token.length < 1) {
    [resultPromise reject:[FIRInstallationsErrorUtil corruptedIIDTokenData]];
    return resultPromise;
  }

  [resultPromise fulfill:IIDTokenInfo.token];

  return resultPromise;
}

- (FBLPromise<NSData *> *)IIDDefaultTokenData {
  FBLPromise<NSData *> *resultPromise = [FBLPromise pendingPromise];

  NSMutableDictionary *keychainQuery = [self IIDDefaultTokenDataKeychainQuery];
  NSError *error;
  NSData *data = [GULKeychainUtils getItemWithQuery:keychainQuery error:&error];

  if (data) {
    [resultPromise fulfill:data];
    return resultPromise;
  } else {
    NSError *outError = error ?: [FIRInstallationsErrorUtil corruptedIIDTokenData];
    [resultPromise reject:outError];
    return resultPromise;
  }
}

- (NSMutableDictionary *)IIDDefaultTokenDataKeychainQuery {
  NSDictionary *query = @{(__bridge id)kSecClass : (__bridge id)kSecClassGenericPassword};

  NSMutableDictionary *finalQuery = [NSMutableDictionary dictionaryWithDictionary:query];
  finalQuery[(__bridge NSString *)kSecAttrGeneric] = kFIRInstallationsIIDTokenKeychainId;

  NSString *account = [self IIDAppIdentifier];
  if ([account length]) {
    finalQuery[(__bridge NSString *)kSecAttrAccount] = account;
  }

  finalQuery[(__bridge NSString *)kSecAttrService] =
      [self serviceKeyForAuthorizedEntity:self.GCMSenderID scope:@"*"];
  return finalQuery;
}

- (NSString *)IIDAppIdentifier {
  return [[NSBundle mainBundle] bundleIdentifier] ?: @"";
}

- (NSString *)serviceKeyForAuthorizedEntity:(NSString *)authorizedEntity scope:(NSString *)scope {
  return [NSString stringWithFormat:@"%@:%@", authorizedEntity, scope];
}

@end
