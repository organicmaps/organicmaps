//
//  PFRelation.h
//
//  Copyright 2011-present Parse Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#if TARGET_OS_IPHONE
#import <Parse/PFNullability.h>
#import <Parse/PFObject.h>
#import <Parse/PFQuery.h>
#else
#import <ParseOSX/PFNullability.h>
#import <ParseOSX/PFObject.h>
#import <ParseOSX/PFQuery.h>
#endif

PF_ASSUME_NONNULL_BEGIN

/*!
 The `PFRelation` class that is used to access all of the children of a many-to-many relationship.
 Each instance of `PFRelation` is associated with a particular parent object and key.
 */
@interface PFRelation : NSObject

/*!
 @abstract The name of the class of the target child objects.
 */
@property (nonatomic, strong) NSString *targetClass;

///--------------------------------------
/// @name Accessing Objects
///--------------------------------------

/*!
 @abstract Returns a <PFQuery> object that can be used to get objects in this relation.
 */
- (PF_NULLABLE PFQuery *)query;

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

PF_ASSUME_NONNULL_END
