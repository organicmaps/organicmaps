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

extern NSString *const FIRCLSUserDefaultsPathComponent;

@interface FIRCLSUserDefaults : NSObject

+ (instancetype)standardUserDefaults;

- (id)objectForKey:(NSString *)key;
- (NSString *)stringForKey:(NSString *)key;
- (BOOL)boolForKey:(NSString *)key;
- (NSInteger)integerForKey:(NSString *)key;

- (void)setObject:(id)object forKey:(NSString *)key;
- (void)setString:(NSString *)string forKey:(NSString *)key;
- (void)setBool:(BOOL)boolean forKey:(NSString *)key;
- (void)setInteger:(NSInteger)integer forKey:(NSString *)key;

- (void)removeObjectForKey:(NSString *)key;
- (void)removeAllObjects;

- (NSDictionary *)dictionaryRepresentation;

- (void)migrateFromNSUserDefaults:(NSArray *)keysToMigrate;
- (id)objectForKeyByMigratingFromNSUserDefaults:(NSString *)keyToMigrateOrNil;
- (void)synchronize;

@end
