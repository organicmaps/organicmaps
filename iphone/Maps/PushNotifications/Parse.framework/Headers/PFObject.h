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

#import <Parse/PFACL.h>
#import <Parse/PFConstants.h>

NS_ASSUME_NONNULL_BEGIN

@protocol PFSubclassing;
@class PFRelation;

/**
 The name of the default pin that for PFObject local data store.
 */
extern NSString *const PFObjectDefaultPin;

/**
 The `PFObject` class is a local representation of data persisted to the Parse cloud.
 This is the main class that is used to interact with objects in your app.
 */
NS_REQUIRES_PROPERTY_DEFINITIONS
@interface PFObject : NSObject

///--------------------------------------
/// @name Creating a PFObject
///--------------------------------------

/**
 Initializes a new empty `PFObject` instance with a class name.

 @param newClassName A class name can be any alphanumeric string that begins with a letter.
 It represents an object in your app, like a 'User' or a 'Document'.

 @return Returns the object that is instantiated with the given class name.
 */
- (instancetype)initWithClassName:(NSString *)newClassName;

/**
 Creates a new PFObject with a class name.

 @param className A class name can be any alphanumeric string that begins with a letter.
 It represents an object in your app, like a 'User' or a 'Document'.

 @return Returns the object that is instantiated with the given class name.
 */
+ (instancetype)objectWithClassName:(NSString *)className;

/**
 Creates a new `PFObject` with a class name, initialized with data
 constructed from the specified set of objects and keys.

 @param className The object's class.
 @param dictionary An `NSDictionary` of keys and objects to set on the new `PFObject`.

 @return A PFObject with the given class name and set with the given data.
 */
+ (instancetype)objectWithClassName:(NSString *)className dictionary:(nullable NSDictionary PF_GENERIC(NSString *, id)*)dictionary;

/**
 Creates a reference to an existing PFObject for use in creating associations between PFObjects.

 Calling `dataAvailable` on this object will return `NO` until `-fetchIfNeeded` has been called.
 No network request will be made.

 @param className The object's class.
 @param objectId The object id for the referenced object.

 @return A `PFObject` instance without data.
 */
+ (instancetype)objectWithoutDataWithClassName:(NSString *)className objectId:(nullable NSString *)objectId;

///--------------------------------------
/// @name Managing Object Properties
///--------------------------------------

/**
 The class name of the object.
 */
@property (nonatomic, strong, readonly) NSString *parseClassName;

/**
 The id of the object.
 */
@property (nullable, nonatomic, strong) NSString *objectId;

/**
 When the object was last updated.
 */
@property (nullable, nonatomic, strong, readonly) NSDate *updatedAt;

/**
 When the object was created.
 */
@property (nullable, nonatomic, strong, readonly) NSDate *createdAt;

/**
 The ACL for this object.
 */
@property (nullable, nonatomic, strong) PFACL *ACL;

/**
 Returns an array of the keys contained in this object.

 This does not include `createdAt`, `updatedAt`, `authData`, or `objectId`.
 It does include things like username and ACL.
 */
@property (nonatomic, copy, readonly) NSArray PF_GENERIC(NSString *)*allKeys;

///--------------------------------------
/// @name Accessors
///--------------------------------------

/**
 Returns the value associated with a given key.

 @param key The key for which to return the corresponding value.

 @see -objectForKeyedSubscript:
 */
- (nullable id)objectForKey:(NSString *)key;

/**
 Sets the object associated with a given key.

 @param object The object for `key`. A strong reference to the object is maintained by PFObject.
 Raises an `NSInvalidArgumentException` if `object` is `nil`.
 If you need to represent a `nil` value - use `NSNull`.
 @param key The key for `object`.
 Raises an `NSInvalidArgumentException` if `key` is `nil`.

 @see -setObject:forKeyedSubscript:
 */
- (void)setObject:(id)object forKey:(NSString *)key;

/**
 Unsets a key on the object.

 @param key The key.
 */
- (void)removeObjectForKey:(NSString *)key;

/**
 Returns the value associated with a given key.

 This method enables usage of literal syntax on `PFObject`.
 E.g. `NSString *value = object[@"key"];`

 @param key The key for which to return the corresponding value.

 @see -objectForKey:
 */
- (nullable id)objectForKeyedSubscript:(NSString *)key;

/**
 Returns the value associated with a given key.

 This method enables usage of literal syntax on `PFObject`.
 E.g. `object[@"key"] = @"value";`

 @param object The object for `key`. A strong reference to the object is maintained by PFObject.
 Raises an `NSInvalidArgumentException` if `object` is `nil`.
 If you need to represent a `nil` value - use `NSNull`.
 @param key The key for `object`.
 Raises an `NSInvalidArgumentException` if `key` is `nil`.

 @see -setObject:forKey:
 */
- (void)setObject:(id)object forKeyedSubscript:(NSString *)key;

/**
 Returns the instance of `PFRelation` class associated with the given key.

 @param key The key that the relation is associated with.
 */
- (PFRelation *)relationForKey:(NSString *)key;

/**
 Returns the instance of `PFRelation` class associated with the given key.

 @param key The key that the relation is associated with.

 @deprecated Please use `PFObject.-relationForKey:` instead.
 */
- (PFRelation *)relationforKey:(NSString *)key PARSE_DEPRECATED("Please use -relationForKey: instead.");

/**
 Clears any changes to this object made since the last call to save and sets it back to the server state.
 */
- (void)revert;

/**
 Clears any changes to this object's key that were done after last successful save and sets it back to the
 server state.

 @param key The key to revert changes for.
 */
- (void)revertObjectForKey:(NSString *)key;

///--------------------------------------
/// @name Array Accessors
///--------------------------------------

/**
 Adds an object to the end of the array associated with a given key.

 @param object The object to add.
 @param key The key.
 */
- (void)addObject:(id)object forKey:(NSString *)key;

/**
 Adds the objects contained in another array to the end of the array associated with a given key.

 @param objects The array of objects to add.
 @param key The key.
 */
- (void)addObjectsFromArray:(NSArray *)objects forKey:(NSString *)key;

/**
 Adds an object to the array associated with a given key, only if it is not already present in the array.

 The position of the insert is not guaranteed.

 @param object The object to add.
 @param key The key.
 */
- (void)addUniqueObject:(id)object forKey:(NSString *)key;

/**
 Adds the objects contained in another array to the array associated with a given key,
 only adding elements which are not already present in the array.

 @dicsussion The position of the insert is not guaranteed.

 @param objects The array of objects to add.
 @param key The key.
 */
- (void)addUniqueObjectsFromArray:(NSArray *)objects forKey:(NSString *)key;

/**
 Removes all occurrences of an object from the array associated with a given key.

 @param object The object to remove.
 @param key The key.
 */
- (void)removeObject:(id)object forKey:(NSString *)key;

/**
 Removes all occurrences of the objects contained in another array from the array associated with a given key.

 @param objects The array of objects to remove.
 @param key The key.
 */
- (void)removeObjectsInArray:(NSArray *)objects forKey:(NSString *)key;

///--------------------------------------
/// @name Increment
///--------------------------------------

/**
 Increments the given key by `1`.

 @param key The key.
 */
- (void)incrementKey:(NSString *)key;

/**
 Increments the given key by a number.

 @param key The key.
 @param amount The amount to increment.
 */
- (void)incrementKey:(NSString *)key byAmount:(NSNumber *)amount;

///--------------------------------------
/// @name Saving Objects
///--------------------------------------

/**
 *Synchronously* saves the `PFObject`.

 @return Returns whether the save succeeded.
 */
- (BOOL)save PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* saves the `PFObject` and sets an error if it occurs.

 @param error Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the save succeeded.
 */
- (BOOL)save:(NSError **)error;

/**
 Saves the `PFObject` *asynchronously*.

 @return The task that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(NSNumber *)*)saveInBackground;

/**
 Saves the `PFObject` *asynchronously* and executes the given callback block.

 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
- (void)saveInBackgroundWithBlock:(nullable PFBooleanResultBlock)block;

/*
 Saves the `PFObject` asynchronously and calls the given callback.

 @param target The object to call selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)result error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `[result boolValue]` will tell you whether the call succeeded or not.
 */
- (void)saveInBackgroundWithTarget:(nullable id)target selector:(nullable SEL)selector;

/**
 Saves this object to the server at some unspecified time in the future,
 even if Parse is currently inaccessible.

 Use this when you may not have a solid network connection, and don't need to know when the save completes.
 If there is some problem with the object such that it can't be saved, it will be silently discarded. If the save
 completes successfully while the object is still in memory, then callback will be called.

 Objects saved with this method will be stored locally in an on-disk cache until they can be delivered to Parse.
 They will be sent immediately if possible. Otherwise, they will be sent the next time a network connection is
 available. Objects saved this way will persist even after the app is closed, in which case they will be sent the
 next time the app is opened. If more than 10MB of data is waiting to be sent, subsequent calls to `-saveEventually`
 will cause old saves to be silently discarded until the connection can be re-established, and the queued objects
 can be saved.

 @return The task that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(NSNumber *)*)saveEventually PF_TV_UNAVAILABLE PF_WATCH_UNAVAILABLE;

/**
 Saves this object to the server at some unspecified time in the future,
 even if Parse is currently inaccessible.

 Use this when you may not have a solid network connection, and don't need to know when the save completes.
 If there is some problem with the object such that it can't be saved, it will be silently discarded. If the save
 completes successfully while the object is still in memory, then callback will be called.

 Objects saved with this method will be stored locally in an on-disk cache until they can be delivered to Parse.
 They will be sent immediately if possible. Otherwise, they will be sent the next time a network connection is
 available. Objects saved this way will persist even after the app is closed, in which case they will be sent the
 next time the app is opened. If more than 10MB of data is waiting to be sent, subsequent calls to `-saveEventually:`
 will cause old saves to be silently discarded until the connection can be re-established, and the queued objects
 can be saved.

 @param callback The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
- (void)saveEventually:(nullable PFBooleanResultBlock)callback PF_TV_UNAVAILABLE PF_WATCH_UNAVAILABLE;

///--------------------------------------
/// @name Saving Many Objects
///--------------------------------------

/**
 Saves a collection of objects *synchronously all at once.

 @param objects The array of objects to save.

 @return Returns whether the save succeeded.
 */
+ (BOOL)saveAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects PF_SWIFT_UNAVAILABLE;

/**
 Saves a collection of objects *synchronously* all at once and sets an error if necessary.

 @param objects The array of objects to save.
 @param error Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the save succeeded.
 */
+ (BOOL)saveAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects error:(NSError **)error;

/**
 Saves a collection of objects all at once *asynchronously*.

 @param objects The array of objects to save.

 @return The task that encapsulates the work being done.
 */
+ (BFTask PF_GENERIC(NSNumber *)*)saveAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects;

/**
 Saves a collection of objects all at once `asynchronously` and executes the block when done.

 @param objects The array of objects to save.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)saveAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects
                      block:(nullable PFBooleanResultBlock)block;

/*
 Saves a collection of objects all at once *asynchronously* and calls a callback when done.

 @param objects The array of objects to save.
 @param target The object to call selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)number error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `[result boolValue]` will tell you whether the call succeeded or not.
 */
+ (void)saveAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects
                     target:(nullable id)target
                   selector:(nullable SEL)selector;

///--------------------------------------
/// @name Deleting Many Objects
///--------------------------------------

/**
 *Synchronously* deletes a collection of objects all at once.

 @param objects The array of objects to delete.

 @return Returns whether the delete succeeded.
 */
+ (BOOL)deleteAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* deletes a collection of objects all at once and sets an error if necessary.

 @param objects The array of objects to delete.
 @param error Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the delete succeeded.
 */
+ (BOOL)deleteAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects error:(NSError **)error;

/**
 Deletes a collection of objects all at once asynchronously.
 @param objects The array of objects to delete.
 @return The task that encapsulates the work being done.
 */
+ (BFTask PF_GENERIC(NSNumber *)*)deleteAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects;

/**
 Deletes a collection of objects all at once *asynchronously* and executes the block when done.

 @param objects The array of objects to delete.
 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)deleteAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects
                        block:(nullable PFBooleanResultBlock)block;

/*
 Deletes a collection of objects all at once *asynchronously* and calls a callback when done.

 @param objects The array of objects to delete.
 @param target The object to call selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)number error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `[result boolValue]` will tell you whether the call succeeded or not.
 */
+ (void)deleteAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects
                       target:(nullable id)target
                     selector:(nullable SEL)selector;

///--------------------------------------
/// @name Getting an Object
///--------------------------------------

/**
 Gets whether the `PFObject` has been fetched.

 @return `YES` if the PFObject is new or has been fetched or refreshed, otherwise `NO`.
 */
@property (nonatomic, assign, readonly, getter=isDataAvailable) BOOL dataAvailable;

#if PARSE_IOS_ONLY

/**
 Refreshes the PFObject with the current data from the server.

 @deprecated Please use `-fetch` instead.
 */
- (nullable instancetype)refresh PF_SWIFT_UNAVAILABLE PARSE_DEPRECATED("Please use `-fetch` instead.");

/**
 *Synchronously* refreshes the `PFObject` with the current data from the server and sets an error if it occurs.

 @param error Pointer to an `NSError` that will be set if necessary.

 @deprecated Please use `-fetch:` instead.
 */
- (nullable instancetype)refresh:(NSError **)error PARSE_DEPRECATED("Please use `-fetch:` instead.");

/**
 *Asynchronously* refreshes the `PFObject` and executes the given callback block.

 @param block The block to execute.
 The block should have the following argument signature: `^(PFObject *object, NSError *error)`

 @deprecated Please use `-fetchInBackgroundWithBlock:` instead.
 */
- (void)refreshInBackgroundWithBlock:(nullable PFObjectResultBlock)block PARSE_DEPRECATED("Please use `-fetchInBackgroundWithBlock:` instead.");

/*
 *Asynchronously* refreshes the `PFObject` and calls the given callback.

 @param target The target on which the selector will be called.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(PFObject *)refreshedObject error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `refreshedObject` will be the `PFObject` with the refreshed data.

 @deprecated Please use `fetchInBackgroundWithTarget:selector:` instead.
 */
- (void)refreshInBackgroundWithTarget:(nullable id)target
                             selector:(nullable SEL)selector PARSE_DEPRECATED("Please use `fetchInBackgroundWithTarget:selector:` instead.");

#endif

/**
 *Synchronously* fetches the PFObject with the current data from the server.
 */
- (nullable instancetype)fetch PF_SWIFT_UNAVAILABLE;
/**
 *Synchronously* fetches the PFObject with the current data from the server and sets an error if it occurs.

 @param error Pointer to an `NSError` that will be set if necessary.
 */
- (nullable instancetype)fetch:(NSError **)error;

/**
 *Synchronously* fetches the `PFObject` data from the server if `dataAvailable` is `NO`.
 */
- (nullable instancetype)fetchIfNeeded PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* fetches the `PFObject` data from the server if `dataAvailable` is `NO`.

 @param error Pointer to an `NSError` that will be set if necessary.
 */
- (nullable instancetype)fetchIfNeeded:(NSError **)error;

/**
 Fetches the `PFObject` *asynchronously* and sets it as a result for the task.

 @return The task that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(__kindof PFObject *)*)fetchInBackground;

/**
 Fetches the `PFObject` *asynchronously* and executes the given callback block.

 @param block The block to execute.
 It should have the following argument signature: `^(PFObject *object, NSError *error)`.
 */
- (void)fetchInBackgroundWithBlock:(nullable PFObjectResultBlock)block;

/*
 Fetches the `PFObject *asynchronously* and calls the given callback.

 @param target The target on which the selector will be called.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(PFObject *)refreshedObject error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `refreshedObject` will be the `PFObject` with the refreshed data.
 */
- (void)fetchInBackgroundWithTarget:(nullable id)target selector:(nullable SEL)selector;

/**
 Fetches the `PFObject` data *asynchronously* if `dataAvailable` is `NO`,
 then sets it as a result for the task.

 @return The task that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(__kindof PFObject *)*)fetchIfNeededInBackground;

/**
 Fetches the `PFObject` data *asynchronously* if `dataAvailable` is `NO`, then calls the callback block.

 @param block The block to execute.
 It should have the following argument signature: `^(PFObject *object, NSError *error)`.
 */
- (void)fetchIfNeededInBackgroundWithBlock:(nullable PFObjectResultBlock)block;

/*
 Fetches the PFObject's data asynchronously if `dataAvailable` is `NO`, then calls the callback.

 @param target The target on which the selector will be called.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(PFObject *)fetchedObject error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `refreshedObject` will be the `PFObject` with the refreshed data.
 */
- (void)fetchIfNeededInBackgroundWithTarget:(nullable id)target selector:(nullable SEL)selector;

///--------------------------------------
/// @name Getting Many Objects
///--------------------------------------

/**
 *Synchronously* fetches all of the `PFObject` objects with the current data from the server.

 @param objects The list of objects to fetch.
 */
+ (nullable NSArray PF_GENERIC(__kindof PFObject *)*)fetchAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* fetches all of the `PFObject` objects with the current data from the server
 and sets an error if it occurs.

 @param objects The list of objects to fetch.
 @param error Pointer to an `NSError` that will be set if necessary.
 */
+ (nullable NSArray PF_GENERIC(__kindof PFObject *)*)fetchAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects
                                                        error:(NSError **)error;

/**
 *Synchronously* fetches all of the `PFObject` objects with the current data from the server.
 @param objects The list of objects to fetch.
 */
+ (nullable NSArray PF_GENERIC(__kindof PFObject *)*)fetchAllIfNeeded:(nullable NSArray PF_GENERIC(PFObject *)*)objects PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* fetches all of the `PFObject` objects with the current data from the server
 and sets an error if it occurs.

 @param objects The list of objects to fetch.
 @param error Pointer to an `NSError` that will be set if necessary.
 */
+ (nullable NSArray PF_GENERIC(__kindof PFObject *)*)fetchAllIfNeeded:(nullable NSArray PF_GENERIC(PFObject *)*)objects
                                                                error:(NSError **)error;

/**
 Fetches all of the `PFObject` objects with the current data from the server *asynchronously*.

 @param objects The list of objects to fetch.

 @return The task that encapsulates the work being done.
 */
+ (BFTask PF_GENERIC(NSArray<__kindof PFObject *> *)*)fetchAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects;

/**
 Fetches all of the `PFObject` objects with the current data from the server *asynchronously*
 and calls the given block.

 @param objects The list of objects to fetch.
 @param block The block to execute.
 It should have the following argument signature: `^(NSArray *objects, NSError *error)`.
 */
+ (void)fetchAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects
                       block:(nullable PFArrayResultBlock)block;

/*
 Fetches all of the `PFObject` objects with the current data from the server *asynchronously*
 and calls the given callback.

 @param objects The list of objects to fetch.
 @param target The target on which the selector will be called.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSArray *)fetchedObjects error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `fetchedObjects` will the array of `PFObject` objects that were fetched.
 */
+ (void)fetchAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects
                      target:(nullable id)target
                    selector:(nullable SEL)selector;

/**
 Fetches all of the `PFObject` objects with the current data from the server *asynchronously*.

 @param objects The list of objects to fetch.

 @return The task that encapsulates the work being done.
 */
+ (BFTask PF_GENERIC(NSArray<__kindof PFObject *> *)*)fetchAllIfNeededInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects;

/**
 Fetches all of the PFObjects with the current data from the server *asynchronously*
 and calls the given block.

 @param objects The list of objects to fetch.
 @param block The block to execute.
 It should have the following argument signature: `^(NSArray *objects, NSError *error)`.
 */
+ (void)fetchAllIfNeededInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects
                               block:(nullable PFArrayResultBlock)block;

/*
 Fetches all of the PFObjects with the current data from the server *asynchronously*
 and calls the given callback.

 @param objects The list of objects to fetch.
 @param target The target on which the selector will be called.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSArray *)fetchedObjects error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `fetchedObjects` will the array of `PFObject` objects that were fetched.
 */
+ (void)fetchAllIfNeededInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects
                              target:(nullable id)target
                            selector:(nullable SEL)selector;

///--------------------------------------
/// @name Fetching From Local Datastore
///--------------------------------------

/**
 *Synchronously* loads data from the local datastore into this object,
 if it has not been fetched from the server already.
 */
- (nullable instancetype)fetchFromLocalDatastore PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* loads data from the local datastore into this object, if it has not been fetched
 from the server already.

 If the object is not stored in the local datastore, this `error` will be set to
 return `kPFErrorCacheMiss`.

 @param error Pointer to an `NSError` that will be set if necessary.
 */
- (nullable instancetype)fetchFromLocalDatastore:(NSError **)error;

/**
 *Asynchronously* loads data from the local datastore into this object,
 if it has not been fetched from the server already.

 @return The task that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(__kindof PFObject *)*)fetchFromLocalDatastoreInBackground;

/**
 *Asynchronously* loads data from the local datastore into this object,
 if it has not been fetched from the server already.

 @param block The block to execute.
 It should have the following argument signature: `^(PFObject *object, NSError *error)`.
 */
- (void)fetchFromLocalDatastoreInBackgroundWithBlock:(nullable PFObjectResultBlock)block;

///--------------------------------------
/// @name Deleting an Object
///--------------------------------------

/**
 *Synchronously* deletes the `PFObject`.

 @return Returns whether the delete succeeded.
 */
- (BOOL)delete PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* deletes the `PFObject` and sets an error if it occurs.

 @param error Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the delete succeeded.
 */
- (BOOL)delete:(NSError **)error;

/**
 Deletes the `PFObject` *asynchronously*.

 @return The task that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(NSNumber *)*)deleteInBackground;

/**
 Deletes the `PFObject` *asynchronously* and executes the given callback block.

 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
- (void)deleteInBackgroundWithBlock:(nullable PFBooleanResultBlock)block;

/*
 Deletes the `PFObject` *asynchronously* and calls the given callback.

 @param target The object to call selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)result error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `[result boolValue]` will tell you whether the call succeeded or not.
 */
- (void)deleteInBackgroundWithTarget:(nullable id)target
                            selector:(nullable SEL)selector;

/**
 Deletes this object from the server at some unspecified time in the future,
 even if Parse is currently inaccessible.

 Use this when you may not have a solid network connection,
 and don't need to know when the delete completes. If there is some problem with the object
 such that it can't be deleted, the request will be silently discarded.

 Delete instructions made with this method will be stored locally in an on-disk cache until they can be transmitted
 to Parse. They will be sent immediately if possible. Otherwise, they will be sent the next time a network connection
 is available. Delete requests will persist even after the app is closed, in which case they will be sent the
 next time the app is opened. If more than 10MB of `-saveEventually` or `-deleteEventually` commands are waiting
 to be sent, subsequent calls to `-saveEventually` or `-deleteEventually` will cause old requests to be silently discarded
 until the connection can be re-established, and the queued requests can go through.

 @return The task that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(NSNumber *)*)deleteEventually PF_TV_UNAVAILABLE PF_WATCH_UNAVAILABLE;

///--------------------------------------
/// @name Dirtiness
///--------------------------------------

/**
 Gets whether any key-value pair in this object (or its children)
 has been added/updated/removed and not saved yet.

 @return Returns whether this object has been altered and not saved yet.
 */
@property (nonatomic, assign, readonly, getter=isDirty) BOOL dirty;

/**
 Get whether a value associated with a key has been added/updated/removed and not saved yet.

 @param key The key to check for

 @return Returns whether this key has been altered and not saved yet.
 */
- (BOOL)isDirtyForKey:(NSString *)key;

///--------------------------------------
/// @name Pinning
///--------------------------------------

/**
 *Synchronously* stores the object and every object it points to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `-fetchFromLocalDatastore` on it.

 @return Returns whether the pin succeeded.

 @see `-unpin:`
 @see `PFObjectDefaultPin`
 */
- (BOOL)pin PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* stores the object and every object it points to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `-fetchFromLocalDatastore` on it.

 @param error Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the pin succeeded.

 @see `-unpin:`
 @see `PFObjectDefaultPin`
 */
- (BOOL)pin:(NSError **)error;

/**
 *Synchronously* stores the object and every object it points to in the local datastore, recursively.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `-fetchFromLocalDatastore` on it.

 @param name The name of the pin.

 @return Returns whether the pin succeeded.

 @see `-unpinWithName:`
 */
- (BOOL)pinWithName:(NSString *)name PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* stores the object and every object it points to in the local datastore, recursively.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `-fetchFromLocalDatastore` on it.

 @param name    The name of the pin.
 @param error   Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the pin succeeded.

 @see `-unpinWithName:`
 */
- (BOOL)pinWithName:(NSString *)name
              error:(NSError **)error;

/**
 *Asynchronously* stores the object and every object it points to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `-fetchFromLocalDatastore` on it.

 @return The task that encapsulates the work being done.

 @see `-unpinInBackground`
 @see `PFObjectDefaultPin`
 */
- (BFTask PF_GENERIC(NSNumber *)*)pinInBackground;

/**
 *Asynchronously* stores the object and every object it points to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `-fetchFromLocalDatastore` on it.

 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.

 @see `-unpinInBackgroundWithBlock:`
 @see `PFObjectDefaultPin`
 */
- (void)pinInBackgroundWithBlock:(nullable PFBooleanResultBlock)block;

/**
 *Asynchronously* stores the object and every object it points to in the local datastore, recursively.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `-fetchFromLocalDatastore on it.

 @param name The name of the pin.

 @return The task that encapsulates the work being done.

 @see unpinInBackgroundWithName:
 */
- (BFTask PF_GENERIC(NSNumber *)*)pinInBackgroundWithName:(NSString *)name;

/**
 *Asynchronously* stores the object and every object it points to in the local datastore, recursively.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `-fetchFromLocalDatastore` on it.

 @param name    The name of the pin.
 @param block   The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.

 @see unpinInBackgroundWithName:block:
 */
- (void)pinInBackgroundWithName:(NSString *)name block:(nullable PFBooleanResultBlock)block;

///--------------------------------------
/// @name Pinning Many Objects
///--------------------------------------

/**
 *Synchronously* stores the objects and every object they point to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `fetchFromLocalDatastore:` on it.

 @param objects The objects to be pinned.

 @return Returns whether the pin succeeded.

 @see unpinAll:
 @see PFObjectDefaultPin
 */
+ (BOOL)pinAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* stores the objects and every object they point to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `fetchFromLocalDatastore:` on it.

 @param objects The objects to be pinned.
 @param error   Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the pin succeeded.

 @see unpinAll:error:
 @see PFObjectDefaultPin
 */
+ (BOOL)pinAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects error:(NSError **)error;

/**
 *Synchronously* stores the objects and every object they point to in the local datastore, recursively.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `fetchFromLocalDatastore:` on it.

 @param objects The objects to be pinned.
 @param name    The name of the pin.

 @return Returns whether the pin succeeded.

 @see unpinAll:withName:
 */
+ (BOOL)pinAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects withName:(NSString *)name PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* stores the objects and every object they point to in the local datastore, recursively.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `fetchFromLocalDatastore:` on it.

 @param objects The objects to be pinned.
 @param name    The name of the pin.
 @param error   Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the pin succeeded.

 @see unpinAll:withName:error:
 */
+ (BOOL)pinAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects
      withName:(NSString *)name
         error:(NSError **)error;

/**
 *Asynchronously* stores the objects and every object they point to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `fetchFromLocalDatastore:` on it.

 @param objects The objects to be pinned.

 @return The task that encapsulates the work being done.

 @see unpinAllInBackground:
 @see PFObjectDefaultPin
 */
+ (BFTask PF_GENERIC(NSNumber *)*)pinAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects;

/**
 *Asynchronously* stores the objects and every object they point to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `fetchFromLocalDatastore:` on it.

 @param objects The objects to be pinned.
 @param block   The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.

 @see unpinAllInBackground:block:
 @see PFObjectDefaultPin
 */
+ (void)pinAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects block:(nullable PFBooleanResultBlock)block;

/**
 *Asynchronously* stores the objects and every object they point to in the local datastore, recursively.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `fetchFromLocalDatastore:` on it.

 @param objects     The objects to be pinned.
 @param name        The name of the pin.

 @return The task that encapsulates the work being done.

 @see unpinAllInBackground:withName:
 */
+ (BFTask PF_GENERIC(NSNumber *)*)pinAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects withName:(NSString *)name;

/**
 *Asynchronously* stores the objects and every object they point to in the local datastore, recursively.

 If those other objects have not been fetched from Parse, they will not be stored. However,
 if they have changed data, all the changes will be retained. To get the objects back later, you can
 use a `PFQuery` that uses `PFQuery.-fromLocalDatastore`, or you can create an unfetched pointer with
 `+objectWithoutDataWithClassName:objectId:` and then call `fetchFromLocalDatastore:` on it.

 @param objects     The objects to be pinned.
 @param name        The name of the pin.
 @param block   The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.

 @see unpinAllInBackground:withName:block:
 */
+ (void)pinAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects
                  withName:(NSString *)name
                     block:(nullable PFBooleanResultBlock)block;

///--------------------------------------
/// @name Unpinning
///--------------------------------------

/**
 *Synchronously* removes the object and every object it points to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 @return Returns whether the unpin succeeded.

 @see pin:
 @see PFObjectDefaultPin
 */
- (BOOL)unpin PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* removes the object and every object it points to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 @param error Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the unpin succeeded.

 @see pin:
 @see PFObjectDefaultPin
 */
- (BOOL)unpin:(NSError **)error;

/**
 *Synchronously* removes the object and every object it points to in the local datastore, recursively.

 @param name The name of the pin.

 @return Returns whether the unpin succeeded.

 @see pinWithName:
 */
- (BOOL)unpinWithName:(NSString *)name PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* removes the object and every object it points to in the local datastore, recursively.

 @param name    The name of the pin.
 @param error   Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the unpin succeeded.

 @see pinWithName:error:
 */
- (BOOL)unpinWithName:(NSString *)name
                error:(NSError **)error;

/**
 *Asynchronously* removes the object and every object it points to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 @return The task that encapsulates the work being done.

 @see pinInBackground
 @see PFObjectDefaultPin
 */
- (BFTask PF_GENERIC(NSNumber *)*)unpinInBackground;

/**
 *Asynchronously* removes the object and every object it points to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 @param block The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.

 @see pinInBackgroundWithBlock:
 @see PFObjectDefaultPin
 */
- (void)unpinInBackgroundWithBlock:(nullable PFBooleanResultBlock)block;

/**
 *Asynchronously* removes the object and every object it points to in the local datastore, recursively.

 @param name The name of the pin.

 @return The task that encapsulates the work being done.

 @see pinInBackgroundWithName:
 */
- (BFTask PF_GENERIC(NSNumber *)*)unpinInBackgroundWithName:(NSString *)name;

/**
 *Asynchronously* removes the object and every object it points to in the local datastore, recursively.

 @param name    The name of the pin.
 @param block   The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.

 @see pinInBackgroundWithName:block:
 */
- (void)unpinInBackgroundWithName:(NSString *)name block:(nullable PFBooleanResultBlock)block;

///--------------------------------------
/// @name Unpinning Many Objects
///--------------------------------------

/**
 *Synchronously* removes all objects in the local datastore
 using a default pin name: `PFObjectDefaultPin`.

 @return Returns whether the unpin succeeded.

 @see PFObjectDefaultPin
 */
+ (BOOL)unpinAllObjects PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* removes all objects in the local datastore
 using a default pin name: `PFObjectDefaultPin`.

 @param error   Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the unpin succeeded.

 @see PFObjectDefaultPin
 */
+ (BOOL)unpinAllObjects:(NSError **)error;

/**
 *Synchronously* removes all objects with the specified pin name.

 @param name    The name of the pin.

 @return Returns whether the unpin succeeded.
 */
+ (BOOL)unpinAllObjectsWithName:(NSString *)name PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* removes all objects with the specified pin name.

 @param name    The name of the pin.
 @param error   Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the unpin succeeded.
 */
+ (BOOL)unpinAllObjectsWithName:(NSString *)name
                          error:(NSError **)error;

/**
 *Asynchronously* removes all objects in the local datastore
 using a default pin name: `PFObjectDefaultPin`.

 @return The task that encapsulates the work being done.

 @see PFObjectDefaultPin
 */
+ (BFTask PF_GENERIC(NSNumber *)*)unpinAllObjectsInBackground;

/**
 *Asynchronously* removes all objects in the local datastore
 using a default pin name: `PFObjectDefaultPin`.

 @param block   The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.

 @see PFObjectDefaultPin
 */
+ (void)unpinAllObjectsInBackgroundWithBlock:(nullable PFBooleanResultBlock)block;

/**
 *Asynchronously* removes all objects with the specified pin name.

 @param name    The name of the pin.

 @return The task that encapsulates the work being done.
 */
+ (BFTask PF_GENERIC(NSNumber *)*)unpinAllObjectsInBackgroundWithName:(NSString *)name;

/**
 *Asynchronously* removes all objects with the specified pin name.

 @param name    The name of the pin.
 @param block   The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
+ (void)unpinAllObjectsInBackgroundWithName:(NSString *)name block:(nullable PFBooleanResultBlock)block;

/**
 *Synchronously* removes the objects and every object they point to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 @param objects The objects.

 @return Returns whether the unpin succeeded.

 @see pinAll:
 @see PFObjectDefaultPin
 */
+ (BOOL)unpinAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* removes the objects and every object they point to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 @param objects The objects.
 @param error   Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the unpin succeeded.

 @see pinAll:error:
 @see PFObjectDefaultPin
 */
+ (BOOL)unpinAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects error:(NSError **)error;

/**
 *Synchronously* removes the objects and every object they point to in the local datastore, recursively.

 @param objects The objects.
 @param name    The name of the pin.

 @return Returns whether the unpin succeeded.

 @see pinAll:withName:
 */
+ (BOOL)unpinAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects withName:(NSString *)name PF_SWIFT_UNAVAILABLE;

/**
 *Synchronously* removes the objects and every object they point to in the local datastore, recursively.

 @param objects The objects.
 @param name    The name of the pin.
 @param error   Pointer to an `NSError` that will be set if necessary.

 @return Returns whether the unpin succeeded.

 @see pinAll:withName:error:
 */
+ (BOOL)unpinAll:(nullable NSArray PF_GENERIC(PFObject *)*)objects
        withName:(NSString *)name
           error:(NSError **)error;

/**
 *Asynchronously* removes the objects and every object they point to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 @param objects The objects.

 @return The task that encapsulates the work being done.

 @see pinAllInBackground:
 @see PFObjectDefaultPin
 */
+ (BFTask PF_GENERIC(NSNumber *)*)unpinAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects;

/**
 *Asynchronously* removes the objects and every object they point to in the local datastore, recursively,
 using a default pin name: `PFObjectDefaultPin`.

 @param objects The objects.
 @param block   The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.

 @see pinAllInBackground:block:
 @see PFObjectDefaultPin
 */
+ (void)unpinAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects block:(nullable PFBooleanResultBlock)block;

/**
 *Asynchronously* removes the objects and every object they point to in the local datastore, recursively.

 @param objects The objects.
 @param name    The name of the pin.

 @return The task that encapsulates the work being done.

 @see pinAllInBackground:withName:
 */
+ (BFTask PF_GENERIC(NSNumber *)*)unpinAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects withName:(NSString *)name;

/**
 *Asynchronously* removes the objects and every object they point to in the local datastore, recursively.

 @param objects The objects.
 @param name    The name of the pin.
 @param block   The block to execute.
 It should have the following argument signature: `^(BOOL succeeded, NSError *error)`.

 @see pinAllInBackground:withName:block:
 */
+ (void)unpinAllInBackground:(nullable NSArray PF_GENERIC(PFObject *)*)objects
                    withName:(NSString *)name
                       block:(nullable PFBooleanResultBlock)block;

@end

NS_ASSUME_NONNULL_END
