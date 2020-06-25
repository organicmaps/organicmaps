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

#import "FIRCLSUserDefaults.h"

#import "FIRCLSApplication.h"
#import "FIRCLSLogger.h"

#define CLS_USER_DEFAULTS_SERIAL_DISPATCH_QUEUE "com.crashlytics.CLSUserDefaults.access"
#define CLS_USER_DEFAULTS_SYNC_QUEUE "com.crashlytics.CLSUserDefaults.io"

#define CLS_TARGET_CAN_WRITE_TO_DISK !TARGET_OS_TV

// These values are required to stay the same between versions of the SDK so
// that when end users upgrade, their crashlytics data is still saved on disk.
#if !CLS_TARGET_CAN_WRITE_TO_DISK
static NSString *const FIRCLSNSUserDefaultsDataDictionaryKey =
    @"com.crashlytics.CLSUserDefaults.user-default-key.data-dictionary";
#endif

NSString *const FIRCLSUserDefaultsPathComponent = @"CLSUserDefaults";

/**
 * This class is an isolated re-implementation of NSUserDefaults which isolates our storage
 * from that of our customers. This solves a number of issues we have seen in production, firstly
 * that customers often delete or clear NSUserDefaults, unintentionally deleting our data.
 * Further, we have seen thread safety issues in production with NSUserDefaults, as well as a number
 * of bugs related to accessing NSUserDefaults before the device has been unlocked due to the
 * NSFileProtection of NSUserDefaults.
 */
@interface FIRCLSUserDefaults ()
@property(nonatomic, readwrite) BOOL synchronizeWroteToDisk;
#if CLS_TARGET_CAN_WRITE_TO_DISK
@property(nonatomic, copy, readonly) NSURL *directoryURL;
@property(nonatomic, copy, readonly) NSURL *fileURL;
#endif
@property(nonatomic, copy, readonly)
    NSDictionary *persistedDataDictionary;  // May only be safely accessed on the DictionaryQueue
@property(nonatomic, copy, readonly)
    NSMutableDictionary *dataDictionary;  // May only be safely accessed on the DictionaryQueue
@property(nonatomic, readonly) dispatch_queue_t
    serialDictionaryQueue;  // The queue on which all access to the dataDictionary occurs.
@property(nonatomic, readonly)
    dispatch_queue_t synchronizationQueue;  // The queue on which all disk access occurs.

@end

@implementation FIRCLSUserDefaults

#pragma mark - singleton

+ (instancetype)standardUserDefaults {
  static FIRCLSUserDefaults *standardUserDefaults = nil;
  static dispatch_once_t onceToken;

  dispatch_once(&onceToken, ^{
    standardUserDefaults = [[super allocWithZone:NULL] init];
  });

  return standardUserDefaults;
}

- (id)copyWithZone:(NSZone *)zone {
  return self;
}

- (id)init {
  if (self = [super init]) {
    _serialDictionaryQueue =
        dispatch_queue_create(CLS_USER_DEFAULTS_SERIAL_DISPATCH_QUEUE, DISPATCH_QUEUE_SERIAL);
    _synchronizationQueue =
        dispatch_queue_create(CLS_USER_DEFAULTS_SYNC_QUEUE, DISPATCH_QUEUE_SERIAL);

    dispatch_sync(self.serialDictionaryQueue, ^{
#if CLS_TARGET_CAN_WRITE_TO_DISK
      self->_directoryURL = [self generateDirectoryURL];
      self->_fileURL = [[self->_directoryURL
          URLByAppendingPathComponent:FIRCLSUserDefaultsPathComponent
                          isDirectory:NO] URLByAppendingPathExtension:@"plist"];
#endif
      self->_persistedDataDictionary = [self loadDefaults];
      if (!self->_persistedDataDictionary) {
        self->_persistedDataDictionary = [NSDictionary dictionary];
      }
      self->_dataDictionary = [self->_persistedDataDictionary mutableCopy];
    });
  }
  return self;
}

- (NSURL *)generateDirectoryURL {
  NSURL *directoryBaseURL =
      [[[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory
                                              inDomains:NSUserDomainMask] lastObject];
  NSString *hostAppBundleIdentifier = [self getEscapedAppBundleIdentifier];
  return [self generateDirectoryURLForBaseURL:directoryBaseURL
                      hostAppBundleIdentifier:hostAppBundleIdentifier];
}

- (NSURL *)generateDirectoryURLForBaseURL:(NSURL *)directoryBaseURL
                  hostAppBundleIdentifier:(NSString *)hostAppBundleIdentifier {
  NSURL *directoryURL = directoryBaseURL;
  // On iOS NSApplicationSupportDirectory is contained in the app's bundle. On OSX, it is not (it is
  // ~/Library/Application Support/). On OSX we create a directory
  // ~/Library/Application Support/<app-identifier>/com.crashlytics/ for storing files.
  // Mac App Store review process requires files to be written to
  // ~/Library/Application Support/<app-identifier>/,
  // so ~/Library/Application Support/com.crashlytics/<app-identifier>/ cannot be used.
#if !TARGET_OS_SIMULATOR && !TARGET_OS_EMBEDDED
  if (hostAppBundleIdentifier) {
    directoryURL = [directoryURL URLByAppendingPathComponent:hostAppBundleIdentifier];
  }
#endif
  directoryURL = [directoryURL URLByAppendingPathComponent:@"com.crashlytics"];
  return directoryURL;
}

- (NSString *)getEscapedAppBundleIdentifier {
  return FIRCLSApplicationGetBundleIdentifier();
}

#pragma mark - fetch object

- (id)objectForKey:(NSString *)key {
  __block id result;

  dispatch_sync(self.serialDictionaryQueue, ^{
    result = [self->_dataDictionary objectForKey:key];
  });

  return result;
}

- (NSString *)stringForKey:(NSString *)key {
  id result = [self objectForKey:key];

  if (result != nil && [result isKindOfClass:[NSString class]]) {
    return (NSString *)result;
  } else {
    return nil;
  }
}

- (BOOL)boolForKey:(NSString *)key {
  id result = [self objectForKey:key];
  if (result != nil && [result isKindOfClass:[NSNumber class]]) {
    return [(NSNumber *)result boolValue];
  } else {
    return NO;
  }
}

// Defaults to 0
- (NSInteger)integerForKey:(NSString *)key {
  id result = [self objectForKey:key];
  if (result && [result isKindOfClass:[NSNumber class]]) {
    return [(NSNumber *)result integerValue];
  } else {
    return 0;
  }
}

#pragma mark - set object

- (void)setObject:(id)object forKey:(NSString *)key {
  dispatch_sync(self.serialDictionaryQueue, ^{
    [self->_dataDictionary setValue:object forKey:key];
  });
}

- (void)setString:(NSString *)string forKey:(NSString *)key {
  [self setObject:string forKey:key];
}

- (void)setBool:(BOOL)boolean forKey:(NSString *)key {
  [self setObject:[NSNumber numberWithBool:boolean] forKey:key];
}

- (void)setInteger:(NSInteger)integer forKey:(NSString *)key {
  [self setObject:[NSNumber numberWithInteger:integer] forKey:key];
}

#pragma mark - removing objects

- (void)removeObjectForKey:(NSString *)key {
  dispatch_sync(self.serialDictionaryQueue, ^{
    [self->_dataDictionary removeObjectForKey:key];
  });
}

- (void)removeAllObjects {
  dispatch_sync(self.serialDictionaryQueue, ^{
    [self->_dataDictionary removeAllObjects];
  });
}

#pragma mark - dictionary representation

- (NSDictionary *)dictionaryRepresentation {
  __block NSDictionary *result;

  dispatch_sync(self.serialDictionaryQueue, ^{
    result = [self->_dataDictionary copy];
  });

  return result;
}

#pragma mark - synchronization

- (void)synchronize {
  __block BOOL dirty = NO;

  // only write to the disk if the dictionaries have changed
  dispatch_sync(self.serialDictionaryQueue, ^{
    dirty = ![self->_persistedDataDictionary isEqualToDictionary:self->_dataDictionary];
  });

  _synchronizeWroteToDisk = dirty;
  if (!dirty) {
    return;
  }

  NSDictionary *state = [self dictionaryRepresentation];
  dispatch_sync(self.synchronizationQueue, ^{
#if CLS_TARGET_CAN_WRITE_TO_DISK
    BOOL isDirectory = NO;
    BOOL pathExists = [[NSFileManager defaultManager] fileExistsAtPath:[self->_directoryURL path]
                                                           isDirectory:&isDirectory];

    if (!pathExists) {
      NSError *error;
      if (![[NSFileManager defaultManager] createDirectoryAtURL:self->_directoryURL
                                    withIntermediateDirectories:YES
                                                     attributes:nil
                                                          error:&error]) {
        FIRCLSErrorLog(@"Failed to create directory with error: %@", error);
      }
    }

    if (![state writeToURL:self->_fileURL atomically:YES]) {
      FIRCLSErrorLog(@"Unable to open file for writing at path %@", [self->_fileURL path]);
    } else {
#if TARGET_OS_IOS
      // We disable NSFileProtection on our file in order to allow us to access it even if the
      // device is locked.
      NSError *error;
      if (![[NSFileManager defaultManager]
              setAttributes:@{NSFileProtectionKey : NSFileProtectionNone}
               ofItemAtPath:[self->_fileURL path]
                      error:&error]) {
        FIRCLSErrorLog(@"Error setting NSFileProtection: %@", error);
      }
#endif
    }
#else
        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
        [defaults setObject:state forKey:FIRCLSNSUserDefaultsDataDictionaryKey];
        [defaults synchronize];
#endif
  });

  dispatch_sync(self.serialDictionaryQueue, ^{
    self->_persistedDataDictionary = [self->_dataDictionary copy];
  });
}

- (NSDictionary *)loadDefaults {
  __block NSDictionary *state = nil;
  dispatch_sync(self.synchronizationQueue, ^{
#if CLS_TARGET_CAN_WRITE_TO_DISK
    BOOL isDirectory = NO;
    BOOL fileExists = [[NSFileManager defaultManager] fileExistsAtPath:[self->_fileURL path]
                                                           isDirectory:&isDirectory];

    if (fileExists && !isDirectory) {
      state = [NSDictionary dictionaryWithContentsOfURL:self->_fileURL];
      if (nil == state) {
        FIRCLSErrorLog(@"Failed to read existing UserDefaults file");
      }
    } else if (!fileExists) {
      // No file found. This is expected on first launch.
    } else if (fileExists && isDirectory) {
      FIRCLSErrorLog(@"Found directory where file expected. Removing conflicting directory");

      NSError *error;
      if (![[NSFileManager defaultManager] removeItemAtURL:self->_fileURL error:&error]) {
        FIRCLSErrorLog(@"Error removing conflicting directory: %@", error);
      }
    }
#else
        state = [[NSUserDefaults standardUserDefaults] dictionaryForKey:FIRCLSNSUserDefaultsDataDictionaryKey];
#endif
  });
  return state;
}

#pragma mark - migration

// This method migrates all keys specified from NSUserDefaults to FIRCLSUserDefaults
// To do so, we copy all known key-value pairs into FIRCLSUserDefaults, synchronize it, then
// remove the keys from NSUserDefaults and synchronize it.
- (void)migrateFromNSUserDefaults:(NSArray *)keysToMigrate {
  BOOL didFindKeys = NO;

  // First, copy all of the keysToMigrate which are stored NSUserDefaults
  for (NSString *key in keysToMigrate) {
    id oldValue = [[NSUserDefaults standardUserDefaults] objectForKey:(NSString *)key];
    if (nil != oldValue) {
      didFindKeys = YES;
      [self setObject:oldValue forKey:key];
    }
  }

  if (didFindKeys) {
    // First synchronize FIRCLSUserDefaults such that all keysToMigrate in NSUserDefaults are stored
    // in FIRCLSUserDefaults. At this point, data is duplicated.
    [[FIRCLSUserDefaults standardUserDefaults] synchronize];

    for (NSString *key in keysToMigrate) {
      [[NSUserDefaults standardUserDefaults] removeObjectForKey:(NSString *)key];
    }

    // This should be our last interaction with NSUserDefaults. All data is migrated into
    // FIRCLSUserDefaults
    [[NSUserDefaults standardUserDefaults] synchronize];
  }
}

// This method first queries FIRCLSUserDefaults to see if the key exist, and upon failure,
// searches for the key in NSUserDefaults, and migrates it if found.
- (id)objectForKeyByMigratingFromNSUserDefaults:(NSString *)keyToMigrateOrNil {
  if (!keyToMigrateOrNil) {
    return nil;
  }

  id clsUserDefaultsValue = [self objectForKey:keyToMigrateOrNil];
  if (clsUserDefaultsValue != nil) {
    return clsUserDefaultsValue;  // if the value exists in FIRCLSUserDefaults, return it.
  }

  id oldNSUserDefaultsValue =
      [[NSUserDefaults standardUserDefaults] objectForKey:keyToMigrateOrNil];
  if (!oldNSUserDefaultsValue) {
    return nil;  // if the value also does not exist in NSUserDefaults, return nil.
  }

  // Otherwise, the key exists in NSUserDefaults. Migrate it to FIRCLSUserDefaults
  // and then return the associated value.

  // First store it in FIRCLSUserDefaults so in the event of a crash, data is not lost.
  [self setObject:oldNSUserDefaultsValue forKey:keyToMigrateOrNil];
  [[FIRCLSUserDefaults standardUserDefaults] synchronize];

  [[NSUserDefaults standardUserDefaults] removeObjectForKey:keyToMigrateOrNil];
  [[NSUserDefaults standardUserDefaults] synchronize];

  return oldNSUserDefaultsValue;
}

@end
