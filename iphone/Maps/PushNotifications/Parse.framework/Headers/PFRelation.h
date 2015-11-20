/**
 * Copyright (c) 2015-present, Parse, LLC.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#import <Foundation/Foundation.h>

#import <Parse/PFObject.h>
#import <Parse/PFQuery.h>

NS_ASSUME_NONNULL_BEGIN

/*!
 The `PFRelation` class that is used to access all of the children of a many-to-many relationship.
 Each instance of `PFRelation` is associated with a particular parent object and key.
 */
@interface PFRelation : NSObject

/*!
 @abstract The name of the class of the target child objects.
 */
@property (nullable, nonatomic, copy) NSString *targetClass;

///--------------------------------------
/// @name Accessing Objects
///--------------------------------------

/*!
 @abstract Returns a <PFQuery> object that can be used to get objects in this relation.
 */
- (PFQuery *)query;

///--------------------------------------
/// @name Modifying Relations
///--------------------------------------

/*!
 @abstract Adds a relation to the passed in object.

 @param object A <PFObject> object to add relation to.
 */
- (void)addObject:(PFObject *)object;

/*!
 @abstract Removes a relation to the passed in object.

 @param object A <PFObject> object to add relation to.
 */
- (void)removeObject:(PFObject *)object;

@end

NS_ASSUME_NONNULL_END
