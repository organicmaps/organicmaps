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

NS_ASSUME_NONNULL_BEGIN

/*!
 `PFFile` representes a file of binary data stored on the Parse servers.
 This can be a image, video, or anything else that an application needs to reference in a non-relational way.
 */
@interface PFFile : NSObject

///--------------------------------------
/// @name Creating a PFFile
///--------------------------------------

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

/*!
 @abstract Creates a file with given data. A name will be assigned to it by the server.

 @param data The contents of the new `PFFile`.

 @returns A new `PFFile`.
 */
+ (nullable instancetype)fileWithData:(NSData *)data;

/*!
 @abstract Creates a file with given data and name.

 @param name The name of the new PFFile. The file name must begin with and
 alphanumeric character, and consist of alphanumeric characters, periods,
 spaces, underscores, or dashes.
 @param data The contents of the new `PFFile`.

 @returns A new `PFFile` object.
 */
+ (nullable instancetype)fileWithName:(nullable NSString *)name data:(NSData *)data;

/*!
 @abstract Creates a file with the contents of another file.

 @warning This method raises an exception if the file at path is not accessible
 or if there is not enough disk space left.

 @param name  The name of the new `PFFile`. The file name must begin with and alphanumeric character,
 and consist of alphanumeric characters, periods, spaces, underscores, or dashes.
 @param path  The path to the file that will be uploaded to Parse.

 @returns A new `PFFile` instance.
 */
+ (nullable instancetype)fileWithName:(nullable NSString *)name
                       contentsAtPath:(NSString *)path PF_SWIFT_UNAVAILABLE;

/*!
 @abstract Creates a file with the contents of another file.

 @param name  The name of the new `PFFile`. The file name must begin with and alphanumeric character,
 and consist of alphanumeric characters, periods, spaces, underscores, or dashes.
 @param path  The path to the file that will be uploaded to Parse.
 @param error On input, a pointer to an error object.
 If an error occurs, this pointer is set to an actual error object containing the error information.
 You may specify `nil` for this parameter if you do not want the error information.

 @returns A new `PFFile` instance or `nil` if the error occured.
 */
+ (nullable instancetype)fileWithName:(nullable NSString *)name
                       contentsAtPath:(NSString *)path
                                error:(NSError **)error;

/*!
 @abstract Creates a file with given data, name and content type.

 @warning This method raises an exception if the data supplied is not accessible or could not be saved.

 @param name        The name of the new `PFFile`. The file name must begin with and alphanumeric character,
 and consist of alphanumeric characters, periods, spaces, underscores, or dashes.
 @param data        The contents of the new `PFFile`.
 @param contentType Represents MIME type of the data.

 @returns A new `PFFile` instance.
 */
+ (nullable instancetype)fileWithName:(nullable NSString *)name
                                 data:(NSData *)data
                          contentType:(nullable NSString *)contentType PF_SWIFT_UNAVAILABLE;

/*!
 @abstract Creates a file with given data, name and content type.

 @param name        The name of the new `PFFile`. The file name must begin with and alphanumeric character,
 and consist of alphanumeric characters, periods, spaces, underscores, or dashes.
 @param data        The contents of the new `PFFile`.
 @param contentType Represents MIME type of the data.
 @param error On input, a pointer to an error object.
 If an error occurs, this pointer is set to an actual error object containing the error information.
 You may specify `nil` for this parameter if you do not want the error information.

 @returns A new `PFFile` instance or `nil` if the error occured.
 */
+ (nullable instancetype)fileWithName:(nullable NSString *)name
                                 data:(NSData *)data
                          contentType:(nullable NSString *)contentType
                                error:(NSError **)error;

/*!
 @abstract Creates a file with given data and content type.

 @param data The contents of the new `PFFile`.
 @param contentType Represents MIME type of the data.

 @returns A new `PFFile` object.
 */
+ (instancetype)fileWithData:(NSData *)data contentType:(nullable NSString *)contentType;

///--------------------------------------
/// @name File Properties
///--------------------------------------

/*!
 @abstract The name of the file.

 @discussion Before the file is saved, this is the filename given by
 the user. After the file is saved, that name gets prefixed with a unique
 identifier.
 */
@property (nonatomic, copy, readonly) NSString *name;

/*!
 @abstract The url of the file.
 */
@property (nullable, nonatomic, copy, readonly) NSString *url;

/*!
 @abstract Whether the file has been uploaded for the first time.
 */
@property (nonatomic, assign, readonly, getter=isDirty) BOOL dirty;

///--------------------------------------
/// @name Storing Data with Parse
///--------------------------------------

/*!
 @abstract Saves the file *synchronously*.

 @returns Returns whether the save succeeded.
 */
- (BOOL)save PF_SWIFT_UNAVAILABLE;

/*!
 @abstract Saves the file *synchronously* and sets an error if it occurs.

 @param error Pointer to an `NSError` that will be set if necessary.

 @returns Returns whether the save succeeded.
 */
- (BOOL)save:(NSError **)error;

/*!
 @abstract Saves the file *asynchronously*.

 @returns The task, that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(NSNumber *)*)saveInBackground;

/*!
 @abstract Saves the file *asynchronously*

 @param progressBlock The block should have the following argument signature: `^(int percentDone)`

 @returns The task, that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(NSNumber *)*)saveInBackgroundWithProgressBlock:(nullable PFProgressBlock)progressBlock;

/*!
 @abstract Saves the file *asynchronously* and executes the given block.

 @param block The block should have the following argument signature: `^(BOOL succeeded, NSError *error)`.
 */
- (void)saveInBackgroundWithBlock:(nullable PFBooleanResultBlock)block;

/*!
 @abstract Saves the file *asynchronously* and executes the given block.

 @discussion This method will execute the progressBlock periodically with the percent progress.
 `progressBlock` will get called with `100` before `resultBlock` is called.

 @param block The block should have the following argument signature: `^(BOOL succeeded, NSError *error)`
 @param progressBlock The block should have the following argument signature: `^(int percentDone)`
 */
- (void)saveInBackgroundWithBlock:(nullable PFBooleanResultBlock)block
                    progressBlock:(nullable PFProgressBlock)progressBlock;

/*
 @abstract Saves the file *asynchronously* and calls the given callback.

 @param target The object to call selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSNumber *)result error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 `[result boolValue]` will tell you whether the call succeeded or not.
 */
- (void)saveInBackgroundWithTarget:(nullable id)target selector:(nullable SEL)selector;

///--------------------------------------
/// @name Getting Data from Parse
///--------------------------------------

/*!
 @abstract Whether the data is available in memory or needs to be downloaded.
 */
@property (nonatomic, assign, readonly, getter=isDataAvailable) BOOL dataAvailable;

/*!
 @abstract *Synchronously* gets the data from cache if available or fetches its contents from the network.

 @returns The `NSData` object containing file data. Returns `nil` if there was an error in fetching.
 */
- (nullable NSData *)getData PF_SWIFT_UNAVAILABLE;

/*!
 @abstract This method is like <getData> but avoids ever holding the entire `PFFile` contents in memory at once.

 @discussion This can help applications with many large files avoid memory warnings.

 @returns A stream containing the data. Returns `nil` if there was an error in fetching.
 */
- (nullable NSInputStream *)getDataStream PF_SWIFT_UNAVAILABLE;

/*!
 @abstract *Synchronously* gets the data from cache if available or fetches its contents from the network.
 Sets an error if it occurs.

 @param error Pointer to an `NSError` that will be set if necessary.

 @returns The `NSData` object containing file data. Returns `nil` if there was an error in fetching.
 */
- (nullable NSData *)getData:(NSError **)error;

/*!
 @abstract This method is like <getData> but avoids ever holding the entire `PFFile` contents in memory at once.

 @param error Pointer to an `NSError` that will be set if necessary.

 @returns A stream containing the data. Returns nil if there was an error in
 fetching.
 */
- (nullable NSInputStream *)getDataStream:(NSError **)error;

/*!
 @abstract This method is like <getData> but it fetches asynchronously to avoid blocking the current thread.

 @see getData

 @returns The task, that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(NSData *)*)getDataInBackground;

/*!
 @abstract This method is like <getData> but it fetches asynchronously to avoid blocking the current thread.

 @discussion This can help applications with many large files avoid memory warnings.

 @see getData

 @param progressBlock The block should have the following argument signature: ^(int percentDone)

 @returns The task, that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(NSData *)*)getDataInBackgroundWithProgressBlock:(nullable PFProgressBlock)progressBlock;

/*!
 @abstract This method is like <getDataInBackground> but avoids
 ever holding the entire `PFFile` contents in memory at once.

 @discussion This can help applications with many large files avoid memory warnings.

 @returns The task, that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(NSInputStream *)*)getDataStreamInBackground;

/*!
 @abstract This method is like <getDataStreamInBackground>, but yields a live-updating stream.

 @discussion Instead of <getDataStream>, which yields a stream that can be read from only after the request has
 completed, this method gives you a stream directly written to by the HTTP session. As this stream is not pre-buffered,
 it is strongly advised to use the `NSStreamDelegate` methods, in combination with a run loop, to consume the data in
 the stream, to do proper async file downloading.

 @note You MUST open this stream before reading from it.
 @note Do NOT call <waitUntilFinished> on this task from the main thread. It may result in a deadlock.

 @returns A task that produces a *live* stream that is being written to with the data from the server.
 */
- (BFTask PF_GENERIC(NSInputStream *)*)getDataDownloadStreamInBackground;

/*!
 @abstract This method is like <getDataInBackground> but avoids
 ever holding the entire `PFFile` contents in memory at once.

 @discussion This can help applications with many large files avoid memory warnings.
 @param progressBlock The block should have the following argument signature: ^(int percentDone)

 @returns The task, that encapsulates the work being done.
 */
- (BFTask PF_GENERIC(NSInputStream *)*)getDataStreamInBackgroundWithProgressBlock:(nullable PFProgressBlock)progressBlock;

/*!
 @abstract This method is like <getDataStreamInBackgroundWithProgrssBlock>, but yields a live-updating stream.

 @discussion Instead of <getDataStream>, which yields a stream that can be read from only after the request has
 completed, this method gives you a stream directly written to by the HTTP session. As this stream is not pre-buffered,
 it is strongly advised to use the `NSStreamDelegate` methods, in combination with a run loop, to consume the data in
 the stream, to do proper async file downloading.

 @note You MUST open this stream before reading from it.
 @note Do NOT call <waitUntilFinished> on this task from the main thread. It may result in a deadlock.

 @param progressBlock The block should have the following argument signature: `^(int percentDone)`

 @returns A task that produces a *live* stream that is being written to with the data from the server.
 */
- (BFTask PF_GENERIC(NSInputStream *)*)getDataDownloadStreamInBackgroundWithProgressBlock:(nullable PFProgressBlock)progressBlock;

/*!
 @abstract *Asynchronously* gets the data from cache if available or fetches its contents from the network.

 @param block The block should have the following argument signature: `^(NSData *result, NSError *error)`
 */
- (void)getDataInBackgroundWithBlock:(nullable PFDataResultBlock)block;

/*!
 @abstract This method is like <getDataInBackgroundWithBlock:> but avoids
 ever holding the entire `PFFile` contents in memory at once.

 @discussion This can help applications with many large files avoid memory warnings.

 @param block The block should have the following argument signature: `(NSInputStream *result, NSError *error)`
 */
- (void)getDataStreamInBackgroundWithBlock:(nullable PFDataStreamResultBlock)block;

/*!
 @abstract *Asynchronously* gets the data from cache if available or fetches its contents from the network.

 @discussion This method will execute the progressBlock periodically with the percent progress.
 `progressBlock` will get called with `100` before `resultBlock` is called.

 @param resultBlock The block should have the following argument signature: ^(NSData *result, NSError *error)
 @param progressBlock The block should have the following argument signature: ^(int percentDone)
 */
- (void)getDataInBackgroundWithBlock:(nullable PFDataResultBlock)resultBlock
                       progressBlock:(nullable PFProgressBlock)progressBlock;

/*!
 @abstract This method is like <getDataInBackgroundWithBlock:progressBlock:> but avoids
 ever holding the entire `PFFile` contents in memory at once.

 @discussion This can help applications with many large files avoid memory warnings.

 @param resultBlock The block should have the following argument signature: `^(NSInputStream *result, NSError *error)`.
 @param progressBlock The block should have the following argument signature: `^(int percentDone)`.
 */
- (void)getDataStreamInBackgroundWithBlock:(nullable PFDataStreamResultBlock)resultBlock
                             progressBlock:(nullable PFProgressBlock)progressBlock;

/*
 @abstract *Asynchronously* gets the data from cache if available or fetches its contents from the network.

 @param target The object to call selector on.
 @param selector The selector to call.
 It should have the following signature: `(void)callbackWithResult:(NSData *)result error:(NSError *)error`.
 `error` will be `nil` on success and set if there was an error.
 */
- (void)getDataInBackgroundWithTarget:(nullable id)target selector:(nullable SEL)selector;

/*!
 @abstract *Asynchronously* gets the file path for file from cache if available or fetches its contents from the network.

 @note The file path may change between versions of SDK.
 @note If you overwrite the contents of the file at returned path it will persist those change
 until the file cache is cleared.

 @returns The task, with the result set to `NSString` representation of a file path.
 */
- (BFTask PF_GENERIC(NSString *)*)getFilePathInBackground;

/*!
 @abstract *Asynchronously* gets the file path for file from cache if available or fetches its contents from the network.

 @note The file path may change between versions of SDK.
 @note If you overwrite the contents of the file at returned path it will persist those change
 until the file cache is cleared.

 @param progressBlock The block should have the following argument signature: `^(int percentDone)`.

 @returns The task, with the result set to `NSString` representation of a file path.
 */
- (BFTask PF_GENERIC(NSString *)*)getFilePathInBackgroundWithProgressBlock:(nullable PFProgressBlock)progressBlock;

/*!
 @abstract *Asynchronously* gets the file path for file from cache if available or fetches its contents from the network.

 @note The file path may change between versions of SDK.
 @note If you overwrite the contents of the file at returned path it will persist those change
 until the file cache is cleared.

 @param block The block should have the following argument signature: `^(NSString *filePath, NSError *error)`.
 */
- (void)getFilePathInBackgroundWithBlock:(nullable PFFilePathResultBlock)block;

/*!
 @abstract *Asynchronously* gets the file path for file from cache if available or fetches its contents from the network.

 @note The file path may change between versions of SDK.
 @note If you overwrite the contents of the file at returned path it will persist those change
 until the file cache is cleared.

 @param block The block should have the following argument signature: `^(NSString *filePath, NSError *error)`.
 @param progressBlock The block should have the following argument signature: `^(int percentDone)`.
 */
- (void)getFilePathInBackgroundWithBlock:(nullable PFFilePathResultBlock)block
                           progressBlock:(nullable PFProgressBlock)progressBlock;

///--------------------------------------
/// @name Interrupting a Transfer
///--------------------------------------

/*!
 @abstract Cancels the current request (upload or download of file).
 */
- (void)cancel;

@end

NS_ASSUME_NONNULL_END
