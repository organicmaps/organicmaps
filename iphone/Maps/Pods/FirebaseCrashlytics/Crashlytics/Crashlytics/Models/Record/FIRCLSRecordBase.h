/*
 * Copyright 2020 Google
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

#import <Foundation/Foundation.h>

/**
 * This is the base class to represent the data in the persisted crash (.clsrecord) files.
 * The properties these subclasses are nullable on purpose. If there is an issue reading values
 * from the crash files, continue as if those fields are optional so a report can still be uploaded.
 * That way the issue can potentially be monitored through the backend.
 **/
@interface FIRCLSRecordBase : NSObject

/**
 * Mark the default initializer as unavailable so  the subclasses do not have to add the same line
 **/
- (instancetype)init NS_UNAVAILABLE;

/**
 * All subclasses should define an initializer taking in a dictionary
 **/
- (instancetype)initWithDict:(NSDictionary *)dict;

@end
