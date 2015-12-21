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

@class PFQuery PF_GENERIC(PFGenericObject : PFObject *);

NS_ASSUME_NONNULL_BEGIN

/**
 ### Subclassing Notes

 Developers can subclass `PFObject` for a more native object-oriented class structure.
 Strongly-typed subclasses of `PFObject` must conform to the `PFSubclassing` protocol
 and must call `PFSubclassing.+registerSubclass` before `Parse.+setApplicationId:clientKey:` is called.
 After this it will be returned by `PFQuery` and other `PFObject` factories.

 All methods in `PFSubclassing` except for `PFSubclassing.+parseClassName`
 are already implemented in the `PFObject(Subclass)` category.

 Including `PFObject+Subclass.h` in your implementation file provides these implementations automatically.

 Subclasses support simpler initializers, query syntax, and dynamic synthesizers.
 The following shows an example subclass:

     \@interface MYGame : PFObject <PFSubclassing>

     // Accessing this property is the same as objectForKey:@"title"
     @property (nonatomic, copy) NSString *title;

     + (NSString *)parseClassName;

     @end


     @implementation MYGame

     @dynamic title;

     + (NSString *)parseClassName {
         return @"Game";
     }

     @end


     MYGame *game = [[MYGame alloc] init];
     game.title = @"Bughouse";
     [game saveInBackground];
 */
@interface PFObject (Subclass)

///--------------------------------------
/// @name Methods for Subclasses
///--------------------------------------

/**
 Creates an instance of the registered subclass with this class's `PFSubclassing.+parseClassName`.

 This helps a subclass ensure that it can be subclassed itself.
 For example, `[PFUser object]` will return a `MyUser` object if `MyUser` is a registered subclass of `PFUser`.
 For this reason, `[MyClass object]` is preferred to `[[MyClass alloc] init]`.
 This method can only be called on subclasses which conform to `PFSubclassing`.
 A default implementation is provided by `PFObject` which should always be sufficient.
 */
+ (instancetype)object;

/**
 Creates a reference to an existing `PFObject` for use in creating associations between `PFObjects`.

 Calling `dataAvailable` on this object will return `NO` until `-fetchIfNeeded` or `-fetch` has been called.
 This method can only be called on subclasses which conform to `PFSubclassing`.
 A default implementation is provided by `PFObject` which should always be sufficient.
 No network request will be made.

 @param objectId The object id for the referenced object.

 @return An instance of `PFObject` without data.
 */
+ (instancetype)objectWithoutDataWithObjectId:(nullable NSString *)objectId;

/**
 Registers an Objective-C class for Parse to use for representing a given Parse class.

 Once this is called on a `PFObject` subclass, any `PFObject` Parse creates with a class name
 that matches `[self parseClassName]` will be an instance of subclass.
 This method can only be called on subclasses which conform to `PFSubclassing`.
 A default implementation is provided by `PFObject` which should always be sufficient.
 */
+ (void)registerSubclass;

/**
 Returns a query for objects of type `PFSubclassing.+parseClassName`.

 This method can only be called on subclasses which conform to `PFSubclassing`.
 A default implementation is provided by `PFObject` which should always be sufficient.
 
 @see `PFQuery`
 */
+ (nullable PFQuery *)query;

/**
 Returns a query for objects of type `PFSubclassing.+parseClassName` with a given predicate.

 A default implementation is provided by `PFObject` which should always be sufficient.
 @warning This method can only be called on subclasses which conform to `PFSubclassing`.

 @param predicate The predicate to create conditions from.

 @return An instance of `PFQuery`.

 @see `PFQuery.+queryWithClassName:predicate:`
 */
+ (nullable PFQuery *)queryWithPredicate:(nullable NSPredicate *)predicate;

@end

NS_ASSUME_NONNULL_END
