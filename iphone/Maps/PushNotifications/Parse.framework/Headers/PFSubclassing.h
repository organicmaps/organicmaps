/**
 * Copyright (c) 2015-present, Parse, LLC.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#import <Foundation/Foundation.h>

@class PFQuery PF_GENERIC(PFGenericObject : PFObject *);

NS_ASSUME_NONNULL_BEGIN

/*!
 If a subclass of <PFObject> conforms to `PFSubclassing` and calls <registerSubclass>,
 Parse framework will be able to use that class as the native class for a Parse cloud object.

 Classes conforming to this protocol should subclass <PFObject> and
 include `PFObject+Subclass.h` in their implementation file.
 This ensures the methods in the Subclass category of <PFObject> are exposed in its subclasses only.
 */
@protocol PFSubclassing

/*!
 @abstract Constructs an object of the most specific class known to implement <parseClassName>.

 @discussion This method takes care to help <PFObject> subclasses be subclassed themselves.
 For example, `[PFUser object]` returns a <PFUser> by default but will return an
 object of a registered subclass instead if one is known.
 A default implementation is provided by <PFObject> which should always be sufficient.

 @returns Returns the object that is instantiated.
 */
+ (instancetype)object;

/*!
 @abstract Creates a reference to an existing PFObject for use in creating associations between PFObjects.

 @discussion Calling <PFObject.dataAvailable> on this object will return `NO`
 until <[PFObject fetchIfNeeded]> has been called. No network request will be made.
 A default implementation is provided by PFObject which should always be sufficient.

 @param objectId The object id for the referenced object.

 @returns A new <PFObject> without data.
 */
+ (instancetype)objectWithoutDataWithObjectId:(nullable NSString *)objectId;

/*!
 @abstract The name of the class as seen in the REST API.
 */
+ (NSString *)parseClassName;

/*!
 @abstract Create a query which returns objects of this type.

 @discussion A default implementation is provided by <PFObject> which should always be sufficient.
 */
+ (nullable PFQuery *)query;

/*!
 @abstract Returns a query for objects of this type with a given predicate.

 @discussion A default implementation is provided by <PFObject> which should always be sufficient.

 @param predicate The predicate to create conditions from.

 @returns An instance of <PFQuery>.

 @see [PFQuery queryWithClassName:predicate:]
 */
+ (nullable PFQuery *)queryWithPredicate:(nullable NSPredicate *)predicate;

/*!
 @abstract Lets Parse know this class should be used to instantiate all objects with class type <parseClassName>.

 @warning This method must be called before <[Parse setApplicationId:clientKey:]>
 */
+ (void)registerSubclass;

@end

NS_ASSUME_NONNULL_END
