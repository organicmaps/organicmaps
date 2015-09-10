/**
 * Copyright (c) 2015-present, Parse, LLC.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#import <Foundation/Foundation.h>

#import <Parse/PFNullability.h>
#import <Parse/PFObject.h>
#import <Parse/PFSubclassing.h>

PF_ASSUME_NONNULL_BEGIN

/*!
 The `PFRole` class represents a Role on the Parse server.
 `PFRoles` represent groupings of <PFUser> objects for the purposes of granting permissions
 (e.g. specifying a <PFACL> for a <PFObject>).
 Roles are specified by their sets of child users and child roles,
 all of which are granted any permissions that the parent role has.

 Roles must have a name (which cannot be changed after creation of the role), and must specify an ACL.
 */
@interface PFRole : PFObject <PFSubclassing>

///--------------------------------------
/// @name Creating a New Role
///--------------------------------------

/*!
 @abstract Constructs a new `PFRole` with the given name.
 If no default ACL has been specified, you must provide an ACL for the role.

 @param name The name of the Role to create.
 */
- (instancetype)initWithName:(NSString *)name;

/*!
 @abstract Constructs a new `PFRole` with the given name.

 @param name The name of the Role to create.
 @param acl The ACL for this role. Roles must have an ACL.
 */
- (instancetype)initWithName:(NSString *)name acl:(PF_NULLABLE PFACL *)acl;

/*!
 @abstract Constructs a new `PFRole` with the given name.

 @discussion If no default ACL has been specified, you must provide an ACL for the role.

 @param name The name of the Role to create.
 */
+ (instancetype)roleWithName:(NSString *)name;

/*!
 @abstract Constructs a new `PFRole` with the given name.

 @param name The name of the Role to create.
 @param acl The ACL for this role. Roles must have an ACL.
 */
+ (instancetype)roleWithName:(NSString *)name acl:(PF_NULLABLE PFACL *)acl;

///--------------------------------------
/// @name Role-specific Properties
///--------------------------------------

/*!
 @abstract Gets or sets the name for a role.

 @discussion This value must be set before the role has been saved to the server,
 and cannot be set once the role has been saved.

 @warning A role's name can only contain alphanumeric characters, `_`, `-`, and spaces.
 */
@property (nonatomic, copy) NSString *name;

/*!
 @abstract Gets the <PFRelation> for the <PFUser> objects that are direct children of this role.

 @discussion These users are granted any privileges that this role has been granted
 (e.g. read or write access through ACLs). You can add or remove users from
 the role through this relation.
 */
@property (nonatomic, strong, readonly) PFRelation *users;

/*!
 @abstract Gets the <PFRelation> for the `PFRole` objects that are direct children of this role.

 @discussion These roles' users are granted any privileges that this role has been granted
 (e.g. read or write access through ACLs). You can add or remove child roles
 from this role through this relation.
 */
@property (nonatomic, strong, readonly) PFRelation *roles;

@end

PF_ASSUME_NONNULL_END
