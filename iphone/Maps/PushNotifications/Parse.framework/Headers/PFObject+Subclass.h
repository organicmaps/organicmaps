//
//  PFObject+Subclass.h
//
//  Copyright 2011-present Parse Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#if TARGET_OS_IPHONE
#import <Parse/PFNullability.h>
#import <Parse/PFObject.h>
#else
#import <ParseOSX/PFNullability.h>
#import <ParseOSX/PFObject.h>
#endif

PF_ASSUME_NONNULL_BEGIN

@class PFQuery;

/*!
 ### Subclassing Notes
 
 Developers can subclass `PFObject` for a more native object-oriented class structure.
 Strongly-typed subclasses of `PFObject` must conform to the <PFSubclassing> protocol
 and must call <registerSubclass> before <[Parse setApplicationId:clientKey:]> is called.
 After this it will be returned by <PFQuery> and other `PFObject` factories.

 All methods in <PFSubclassing> except for <[PFSubclassing parseClassName]>
 are already implemented in the `PFObject+Subclass` category.

 Including `PFObject+Subclass.h` in your implementation file provides these implementations automatically.
 
 Subclasses support simpler initializers, query syntax, and dynamic synthesizers.
 The following shows an example subclass:
 
     \@interface MYGame : PFObject <PFSubclassing>

     // Accessing this property is the same as objectForKey:@"title"
     @property (nonatomic, strong) NSString *title;

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

/*!
 @abstract Creates an instance of the registered subclass with this class's <parseClassName>.

 @discussion This helps a subclass ensure that it can be subclassed itself.
 For example, `[PFUser object]` will return a `MyUser` object if `MyUser` is a registered subclass of `PFUser`.
 For this reason, `[MyClass object]` is preferred to `[[MyClass alloc] init]`.
 This method can only be called on subclasses which conform to `PFSubclassing`.
 A default implementation is provided by `PFObject` which should always be sufficient.
 */
+ (instancetype)object;

/*!
 @abstract Creates a reference to an existing `PFObject` for use in creating associations between `PFObjects`.

 @discussion Calling <isDataAvailable> on this object will return `NO` until <fetchIfNeeded> or <fetch> has been called.
 This method can only be called on subclasses which conform to <PFSubclassing>.
 A default implementation is provided by `PFObject` which should always be sufficient.
 No network request will be made.

 @param objectId The object id for the referenced object.

 @returns An instance of `PFObject` without data.
 */
+ (instancetype)objectWithoutDataWithObjectId:(PF_NULLABLE NSString *)objectId;

/*!
 @abstract Registers an Objective-C class for Parse to use for representing a given Parse class.

 @discussion Once this is called on a `PFObject` subclass, any `PFObject` Parse creates with a class name
 that matches `[self parseClassName]` will be an instance of subclass.
 This method can only be called on subclasses which conform to <PFSubclassing>.
 A default implementation is provided by `PFObject` which should always be sufficient.
 */
+ (void)registerSubclass;

/*!
 @abstract Returns a query for objects of type <parseClassName>.

 @discussion This method can only be called on subclasses which conform to <PFSubclassing>.
 A default implementation is provided by <PFObject> which should always be sufficient.
 */
+ (PF_NULLABLE PFQuery *)query;

/*!
 @abstract Returns a query for objects of type <parseClassName> with a given predicate.

 @discussion A default implementation is provided by <PFObject> which should always be sufficient.
 @warning This method can only be called on subclasses which conform to <PFSubclassing>.

 @param predicate The predicate to create conditions from.

 @returns An instance of <PFQuery>.

 @see [PFQuery queryWithClassName:predicate:]
 */
+ (PF_NULLABLE PFQuery *)queryWithPredicate:(PF_NULLABLE NSPredicate *)predicate;

@end

PF_ASSUME_NONNULL_END
