/**
 * Copyright (c) 2015-present, Parse, LLC.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#import <Foundation/Foundation.h>

#import <Bolts/BFTask.h>

#import <Parse/PFConstants.h>
#import <Parse/PFGeoPoint.h>
#import <Parse/PFObject.h>
#import <Parse/PFUser.h>

NS_ASSUME_NONNULL_BEGIN

/**
 The `PFQuery` class defines a query that is used to query for `PFObject`s.
 */
@interface PFQuery PF_GENERIC(PFGenericObject : PFObject *) : NSObject<NSCopying>

///--------------------------------------
/// @name Blocks
///--------------------------------------

typedef void (^PFQueryArrayResultBlock)(NSArray PF_GENERIC(PFGenericObject) * __nullable objects, NSError * __nullable error);

///--------------------------------------
/// @name Creating a Query for a Class
///--------------------------------------

/**
 Initializes the query with a class name.

 @param className The class name.
 */
- (instancetype)initWithClassName:(NSString *)className;

/**
 Returns a `PFQuery` for a given class.

 @param className The class to query on.

 @return A `PFQuery` object.
 */
+ (instancetype)queryWithClassName:(NSString *)className;

/**
 Creates a PFQuery with the constraints given by predicate.

 The following types of predicates are supported:

 - Simple comparisons such as `=`, `!=`, `<`, `>`, `<=`, `>=`, and `BETWEEN` with a key and a constant.
 - Containment predicates, such as `x IN {1, 2, 3}`.
 - Key-existence predicates, such as `x IN SELF`.
 - BEGINSWITH expressions.
 - Compound predicates with `AND`, `OR`, and `NOT`.
 - SubQueries with `key IN %@`, subquery.

 The following types of predicates are NOT supported:

 - Aggregate operations, such as `ANY`, `SOME`, `ALL`, or `NONE`.
 - Regular expressions, such as `LIKE`, `MATCHES`, `CONTAINS`, or `ENDSWITH`.
 - Predicates comparing one key to another.
 - Complex predicates with many ORed clauses.

 @param className The class to query on.
 @param predicate The predicate to create conditions from.
 */
+ (instancetype)queryWithClassName:(NSString *)className predicate:(nullable NSPredicate *)predicate;

/**
 The class name to query for.
 */
@property (nonatomic, strong) NSString *parseClassName;

///--------------------------------------
/// @name Adding Basic Constraints
///--------------------------------------

/**
 Make the query include PFObjects that have a reference stored at the provided key.

 This has an effect similar to a join.  You can use dot notation to specify which fields in
 the included object are also fetch.

 @param key The key to load child `PFObject`s for.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)includeKey:(NSString *)key;

/**
 Make the query restrict the fields of the returned `PFObject`s to include only the provided keys.

 If this is called multiple times, then all of the keys specified in each of the calls will be included.

 @param keys The keys to include in the result.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)selectKeys:(NSArray PF_GENERIC(NSString *)*)keys;

/**
 Add a constraint that requires a particular key exists.

 @param key The key that should exist.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKeyExists:(NSString *)key;

/**
 Add a constraint that requires a key not exist.

 @param key The key that should not exist.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKeyDoesNotExist:(NSString *)key;

/**
 Add a constraint to the query that requires a particular key's object to be equal to the provided object.

 @param key The key to be constrained.
 @param object The object that must be equalled.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key equalTo:(id)object;

/**
 Add a constraint to the query that requires a particular key's object to be less than the provided object.

 @param key The key to be constrained.
 @param object The object that provides an upper bound.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key lessThan:(id)object;

/**
 Add a constraint to the query that requires a particular key's object
 to be less than or equal to the provided object.

 @param key The key to be constrained.
 @param object The object that must be equalled.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key lessThanOrEqualTo:(id)object;

/**
 Add a constraint to the query that requires a particular key's object
 to be greater than the provided object.

 @param key The key to be constrained.
 @param object The object that must be equalled.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key greaterThan:(id)object;

/**
 Add a constraint to the query that requires a particular key's
 object to be greater than or equal to the provided object.

 @param key The key to be constrained.
 @param object The object that must be equalled.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key greaterThanOrEqualTo:(id)object;

/**
 Add a constraint to the query that requires a particular key's object
 to be not equal to the provided object.

 @param key The key to be constrained.
 @param object The object that must not be equalled.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key notEqualTo:(id)object;

/**
 Add a constraint to the query that requires a particular key's object
 to be contained in the provided array.

 @param key The key to be constrained.
 @param array The possible values for the key's object.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key containedIn:(NSArray *)array;

/**
 Add a constraint to the query that requires a particular key's object
 not be contained in the provided array.

 @param key The key to be constrained.
 @param array The list of values the key's object should not be.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key notContainedIn:(NSArray *)array;

/**
 Add a constraint to the query that requires a particular key's array
 contains every element of the provided array.

 @param key The key to be constrained.
 @param array The array of values to search for.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key containsAllObjectsInArray:(NSArray *)array;

///--------------------------------------
/// @name Adding Location Constraints
///--------------------------------------

/**
 Add a constraint to the query that requires a particular key's coordinates (specified via `PFGeoPoint`)
 be near a reference point.

 Distance is calculated based on angular distance on a sphere. Results will be sorted by distance
 from reference point.

 @param key The key to be constrained.
 @param geopoint The reference point represented as a `PFGeoPoint`.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key nearGeoPoint:(PFGeoPoint *)geopoint;

/**
 Add a constraint to the query that requires a particular key's coordinates (specified via `PFGeoPoint`)
 be near a reference point and within the maximum distance specified (in miles).

 Distance is calculated based on a spherical coordinate system.
 Results will be sorted by distance (nearest to farthest) from the reference point.

 @param key The key to be constrained.
 @param geopoint The reference point represented as a `PFGeoPoint`.
 @param maxDistance Maximum distance in miles.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key
            nearGeoPoint:(PFGeoPoint *)geopoint
             withinMiles:(double)maxDistance;

/**
 Add a constraint to the query that requires a particular key's coordinates (specified via `PFGeoPoint`)
 be near a reference point and within the maximum distance specified (in kilometers).

 Distance is calculated based on a spherical coordinate system.
 Results will be sorted by distance (nearest to farthest) from the reference point.

 @param key The key to be constrained.
 @param geopoint The reference point represented as a `PFGeoPoint`.
 @param maxDistance Maximum distance in kilometers.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key
            nearGeoPoint:(PFGeoPoint *)geopoint
        withinKilometers:(double)maxDistance;

/**
 Add a constraint to the query that requires a particular key's coordinates (specified via `PFGeoPoint`) be near
 a reference point and within the maximum distance specified (in radians).  Distance is calculated based on
 angular distance on a sphere.  Results will be sorted by distance (nearest to farthest) from the reference point.

 @param key The key to be constrained.
 @param geopoint The reference point as a `PFGeoPoint`.
 @param maxDistance Maximum distance in radians.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key
            nearGeoPoint:(PFGeoPoint *)geopoint
           withinRadians:(double)maxDistance;

/**
 Add a constraint to the query that requires a particular key's coordinates (specified via `PFGeoPoint`) be
 contained within a given rectangular geographic bounding box.

 @param key The key to be constrained.
 @param southwest The lower-left inclusive corner of the box.
 @param northeast The upper-right inclusive corner of the box.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key withinGeoBoxFromSouthwest:(PFGeoPoint *)southwest toNortheast:(PFGeoPoint *)northeast;

///--------------------------------------
/// @name Adding String Constraints
///--------------------------------------

/**
 Add a regular expression constraint for finding string values that match the provided regular expression.

 @warning This may be slow for large datasets.

 @param key The key that the string to match is stored in.
 @param regex The regular expression pattern to match.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key matchesRegex:(NSString *)regex;

/**
 Add a regular expression constraint for finding string values that match the provided regular expression.

 @warning This may be slow for large datasets.

 @param key The key that the string to match is stored in.
 @param regex The regular expression pattern to match.
 @param modifiers Any of the following supported PCRE modifiers:
 - `i` - Case insensitive search
 - `m` - Search across multiple lines of input

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key
            matchesRegex:(NSString *)regex
               modifiers:(nullable NSString *)modifiers;

/**
 Add a constraint for finding string values that contain a provided substring.

 @warning This will be slow for large datasets.

 @param key The key that the string to match is stored in.
 @param substring The substring that the value must contain.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key containsString:(nullable NSString *)substring;

/**
 Add a constraint for finding string values that start with a provided prefix.

 This will use smart indexing, so it will be fast for large datasets.

 @param key The key that the string to match is stored in.
 @param prefix The substring that the value must start with.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key hasPrefix:(nullable NSString *)prefix;

/**
 Add a constraint for finding string values that end with a provided suffix.

 @warning This will be slow for large datasets.

 @param key The key that the string to match is stored in.
 @param suffix The substring that the value must end with.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key hasSuffix:(nullable NSString *)suffix;

///--------------------------------------
/// @name Adding Subqueries
///--------------------------------------

/**
 Returns a `PFQuery` that is the `or` of the passed in queries.

 @param queries The list of queries to or together.

 @return An instance of `PFQuery` that is the `or` of the passed in queries.
 */
+ (instancetype)orQueryWithSubqueries:(NSArray PF_GENERIC(PFQuery *)*)queries;

/**
 Adds a constraint that requires that a key's value matches a value in another key
 in objects returned by a sub query.

 @param key The key that the value is stored.
 @param otherKey The key in objects in the returned by the sub query whose value should match.
 @param query The query to run.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key
              matchesKey:(NSString *)otherKey
                 inQuery:(PFQuery *)query;

/**
 Adds a constraint that requires that a key's value `NOT` match a value in another key
 in objects returned by a sub query.

 @param key The key that the value is stored.
 @param otherKey The key in objects in the returned by the sub query whose value should match.
 @param query The query to run.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key
         doesNotMatchKey:(NSString *)otherKey
                 inQuery:(PFQuery *)query;

/**
 Add a constraint that requires that a key's value matches a `PFQuery` constraint.

 @warning This only works where the key's values are `PFObject`s or arrays of `PFObject`s.

 @param key The key that the value is stored in
 @param query The query the value should match

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key matchesQuery:(PFQuery *)query;

/**
 Add a constraint that requires that a key's value to not match a `PFQuery` constraint.

 @warning This only works where the key's values are `PFObject`s or arrays of `PFObject`s.

 @param key The key that the value is stored in
 @param query The query the value should not match

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)whereKey:(NSString *)key doesNotMatchQuery:(PFQuery *)query;

///--------------------------------------
/// @name Sorting
///--------------------------------------

/**
 Sort the results in *ascending* order with the given key.

 @param key The key to order by.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)orderByAscending:(NSString *)key;

/**
 Additionally sort in *ascending* order by the given key.

 The previous keys provided will precedence over this key.

 @param key The key to order by.
 */
- (instancetype)addAscendingOrder:(NSString *)key;

/**
 Sort the results in *descending* order with the given key.

 @param key The key to order by.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)orderByDescending:(NSString *)key;

/**
 Additionally sort in *descending* order by the given key.

 The previous keys provided will precedence over this key.

 @param key The key to order by.
 */
- (instancetype)addDescendingOrder:(NSString *)key;

/**
 Sort the results using a given sort descriptor.

 @warning If a `sortDescriptor` has custom `selector` or `comparator` - they aren't going to be used.

 @param sortDescriptor The `NSSortDescriptor` to use to sort the results of the query.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)orderBySortDescriptor:(NSSortDescriptor *)sortDescriptor;

/**
 Sort the results using a given array of sort descriptors.

 @warning If a `sortDescriptor` has custom `selector` or `comparator` - they aren't going to be used.

 @param sortDescriptors An array of `NSSortDescriptor` objects to use to sort the results of the query.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)orderBySortDescriptors:(nullable NSArray PF_GENERIC(NSSortDescriptor *)*)sortDescriptors;

///--------------------------------------
/// @name Getting Objects by ID
///--------------------------------------

/**
 Returns a `PFObject` with a given class and id.

 @param objectClass The class name for the object that is being requested.
 @param objectId The id of the object that is being requested.

 @return The `PFObject` if found. Returns `nil` if the object isn't found, or if there was an error.
 */
+ (nullable PFGenericObject)getObjectOfClass:(NSString *)objectClass
                                    objectId:(NSString *)objectId PF_SWIFT_UNAVAILABLE;

/**
 Returns a `PFObject` with a given class and id and sets an error if necessary.

 @param objectClass The class name for the object that is being requested.
 @param objectId The id of the object that is being requested.
 @param error Pointer to an `NSError` that will be set if necessary.

 @return The `PFObject` if found. Returns `nil` if the object isn't found, or if there was an `error`.
 */
+ (nullable PFGenericObject)getObjectOfClass:(NSString *)objectClass
                                    objectId:(NSString *)objectId
                                       error:(NSError **)error;

/**
 Returns a `PFObject` with the given id.

 @warning This method mutates the query.
 It will reset limit to `1`, skip to `0` and remove all conditions, leaving only `objectId`.

 @param objectId The id of the object that is being requested.

 @return The `PFObject` if found. Returns nil if the object isn't found, or if there was an error.
 */
- (nullable PFGenericObject)getObjectWithId:(NSString *)objectId PF_SWIFT_UNAVAILABLE;

/**
 Returns a `PFObject` with the given id and sets an error if necessary.

 @warning This method mutates the query.
 It will reset limit to `1`, skip to `0` and remove all conditions, leaving only `objectId`.

 @param objectId The id of the object that is being requested.
 @param error Pointer to an `NSError` that will be set if necessary.

 @return The `PFObject` if found. Returns nil if the object isn't found, or if there was an error.
 */
- (nullable PFGenericObject)getObjectWithId:(NSString *)objectId error:(NSError **)error;

/**
 Gets a `PFObject` asynchronously and calls the given block with the result.

 @warning This method mutates the query.
 It will reset limit to `1`, skip to `0` and remove all conditions, leaving only `objectId`.

 @param objectId The id of the object that is being requested.

 @return The task, that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(PFGenericObject) *)getObjectInBackgroundWithId:(NSString *)objectId;

/**
 Gets a `PFObject` asynchronously and calls the given block with the result.

 @warning This method mutates the query.
 It will reset limit to `1`, skip to `0` and remove all conditions, leaving only `objectId`.

 @param objectId The id of the object that is being requested.
 @param block The block to execute.
 The block should have the following argument signature: `^(NSArray *object, NSError *error)`
 */
- (void)getObjectInBackgroundWithId:(NSString *)objectId
                              block:(nullable void (^)(PFGenericObject __nullable object, NSError *__nullable error))block;

/*
 Gets a `PFObject` asynchronously.

 This mutates the PFQuery. It will reset limit to `1`, skip to `0` and remove all conditions, leaving only `objectId`.

 @param objectId The id of the object being requested.
 @param target The target for the callback selector.
 @param selector The selector for the callback.
 It should have the following signature: `(void)callbackWithResult:(id)result error:(NSError *)error`.
 Result will be `nil` if error is set and vice versa.
 */
- (void)getObjectInBackgroundWithId:(NSString *)objectId
                             target:(nullable id)target
                           selector:(nullable SEL)selector;

///--------------------------------------
/// @name Getting User Objects
///--------------------------------------

/**
 Returns a `PFUser` with a given id.

 @param objectId The id of the object that is being requested.

 @return The PFUser if found. Returns nil if the object isn't found, or if there was an error.
 */
+ (nullable PFUser *)getUserObjectWithId:(NSString *)objectId PF_SWIFT_UNAVAILABLE;

/**
 Returns a PFUser with a given class and id and sets an error if necessary.
 @param objectId The id of the object that is being requested.
 @param error Pointer to an NSError that will be set if necessary.
 @result The PFUser if found. Returns nil if the object isn't found, or if there was an error.
 */
+ (nullable PFUser *)getUserObjectWithId:(NSString *)objectId error:(NSError **)error;

/**
 @deprecated Please use [PFUser query] instead.
 */
+ (instancetype)queryForUser PARSE_DEPRECATED("Use [PFUser query] instead.");

///--------------------------------------
/// @name Getting all Matches for a Query
///--------------------------------------

/**
 Finds objects *synchronously* based on the constructed query.

 @return Returns an array of `PFObject` objects that were found.
 */
- (nullable NSArray PF_GENERIC(PFGenericObject) *)findObjects PF_SWIFT_UNAVAILABLE;

/**
 Finds objects *synchronously* based on the constructed query and sets an error if there was one.

 @param error Pointer to an `NSError` that will be set if necessary.

 @return Returns an array of `PFObject` objects that were found.
 */
- (nullable NSArray PF_GENERIC(PFGenericObject) *)findObjects:(NSError **)error;

/**
 Finds objects *asynchronously* and sets the `NSArray` of `PFObject` objects as a result of the task.

 @return The task, that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(NSArray<PFGenericObject> *)*)findObjectsInBackground;

/**
 Finds objects *asynchronously* and calls the given block with the results.

 @param block The block to execute.
 It should have the following argument signature: `^(NSArray *objects, NSError *error)`
 */
- (void)findObjectsInBackgroundWithBlock:(nullable PFQueryArrayResultBlock)block;

/*
 Finds objects *asynchronously* and calls the given callback with the results.

 @param target The object to call the selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(id)result error:(NSError *)error`.
 Result will be `nil` if error is set and vice versa.
 */
- (void)findObjectsInBackgroundWithTarget:(nullable id)target selector:(nullable SEL)selector;

///--------------------------------------
/// @name Getting the First Match in a Query
///--------------------------------------

/**
 Gets an object *synchronously* based on the constructed query.

 @warning This method mutates the query. It will reset the limit to `1`.

 @return Returns a `PFObject`, or `nil` if none was found.
 */
- (nullable PFGenericObject)getFirstObject PF_SWIFT_UNAVAILABLE;

/**
 Gets an object *synchronously* based on the constructed query and sets an error if any occurred.

 @warning This method mutates the query. It will reset the limit to `1`.

 @param error Pointer to an `NSError` that will be set if necessary.

 @return Returns a `PFObject`, or `nil` if none was found.
 */
- (nullable PFGenericObject)getFirstObject:(NSError **)error;

/**
 Gets an object *asynchronously* and sets it as a result of the task.

 @warning This method mutates the query. It will reset the limit to `1`.

 @return The task, that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(PFGenericObject) *)getFirstObjectInBackground;

/**
 Gets an object *asynchronously* and calls the given block with the result.

 @warning This method mutates the query. It will reset the limit to `1`.

 @param block The block to execute.
 It should have the following argument signature: `^(PFObject *object, NSError *error)`.
 `result` will be `nil` if `error` is set OR no object was found matching the query.
 `error` will be `nil` if `result` is set OR if the query succeeded, but found no results.
 */
- (void)getFirstObjectInBackgroundWithBlock:(nullable void (^)(PFGenericObject __nullable object, NSError *__nullable error))block;

/*
 Gets an object *asynchronously* and calls the given callback with the results.

 @warning This method mutates the query. It will reset the limit to `1`.

 @param target The object to call the selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(PFObject *)result error:(NSError *)error`.
 `result` will be `nil` if `error` is set OR no object was found matching the query.
 `error` will be `nil` if `result` is set OR if the query succeeded, but found no results.
 */
- (void)getFirstObjectInBackgroundWithTarget:(nullable id)target selector:(nullable SEL)selector;

///--------------------------------------
/// @name Counting the Matches in a Query
///--------------------------------------

/**
 Counts objects *synchronously* based on the constructed query.

 @return Returns the number of `PFObject` objects that match the query, or `-1` if there is an error.
 */
- (NSInteger)countObjects PF_SWIFT_UNAVAILABLE;

/**
 Counts objects *synchronously* based on the constructed query and sets an error if there was one.

 @param error Pointer to an `NSError` that will be set if necessary.

 @return Returns the number of `PFObject` objects that match the query, or `-1` if there is an error.
 */
- (NSInteger)countObjects:(NSError **)error;

/**
 Counts objects *asynchronously* and sets `NSNumber` with count as a result of the task.

 @return The task, that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(NSNumber *)*)countObjectsInBackground;

/**
 Counts objects *asynchronously* and calls the given block with the counts.

 @param block The block to execute.
 It should have the following argument signature: `^(int count, NSError *error)`
 */
- (void)countObjectsInBackgroundWithBlock:(nullable PFIntegerResultBlock)block;

/*
 Counts objects *asynchronously* and calls the given callback with the count.

 @param target The object to call the selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)result error:(NSError *)error`.
 */
- (void)countObjectsInBackgroundWithTarget:(nullable id)target selector:(nullable SEL)selector;

///--------------------------------------
/// @name Cancelling a Query
///--------------------------------------

/**
 Cancels the current network request (if any). Ensures that callbacks won't be called.
 */
- (void)cancel;

///--------------------------------------
/// @name Paginating Results
///--------------------------------------

/**
 A limit on the number of objects to return. The default limit is `100`, with a
 maximum of 1000 results being returned at a time.

 @warning If you are calling `findObjects` with `limit = 1`, you may find it easier to use `getFirst` instead.
 */
@property (nonatomic, assign) NSInteger limit;

/**
 The number of objects to skip before returning any.
 */
@property (nonatomic, assign) NSInteger skip;

///--------------------------------------
/// @name Controlling Caching Behavior
///--------------------------------------

/**
 The cache policy to use for requests.

 Not allowed when Pinning is enabled.

 @see fromLocalDatastore
 @see fromPin
 @see fromPinWithName:
 */
@property (nonatomic, assign) PFCachePolicy cachePolicy;

/**
 The age after which a cached value will be ignored
 */
@property (nonatomic, assign) NSTimeInterval maxCacheAge;

/**
 Returns whether there is a cached result for this query.

 @result `YES` if there is a cached result for this query, otherwise `NO`.
 */
- (BOOL)hasCachedResult;

/**
 Clears the cached result for this query. If there is no cached result, this is a noop.
 */
- (void)clearCachedResult;

/**
 Clears the cached results for all queries.
 */
+ (void)clearAllCachedResults;

///--------------------------------------
/// @name Query Source
///--------------------------------------

/**
 Change the source of this query to all pinned objects.

 @warning Requires Local Datastore to be enabled.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.

 @see cachePolicy
 */
- (instancetype)fromLocalDatastore;

/**
 Change the source of this query to the default group of pinned objects.

 @warning Requires Local Datastore to be enabled.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.

 @see PFObjectDefaultPin
 @see cachePolicy
 */
- (instancetype)fromPin;

/**
 Change the source of this query to a specific group of pinned objects.

 @warning Requires Local Datastore to be enabled.

 @param name The pinned group.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.

 @see PFObjectDefaultPin
 @see cachePolicy
 */
- (instancetype)fromPinWithName:(nullable NSString *)name;

/**
 Ignore ACLs when querying from the Local Datastore.

 This is particularly useful when querying for objects with Role based ACLs set on them.

 @warning Requires Local Datastore to be enabled.

 @return The same instance of `PFQuery` as the receiver. This allows method chaining.
 */
- (instancetype)ignoreACLs;

///--------------------------------------
/// @name Advanced Settings
///--------------------------------------

/**
 Whether or not performance tracing should be done on the query.

 @warning This should not be set to `YES` in most cases.
 */
@property (nonatomic, assign) BOOL trace;

@end

NS_ASSUME_NONNULL_END
