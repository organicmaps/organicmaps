//
//  MPMediaFileCache.h
//
//  Copyright 2018-2019 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#ifndef MPMediaFileCache_h
#define MPMediaFileCache_h

#import <Foundation/Foundation.h>
#import "MPVASTMediaFile.h"

NS_ASSUME_NONNULL_BEGIN

/**
 @c AVPlayer and related media player API requires the file extension (such as "mp4" and "3gpp")
 being in the file name, otherwise the media file cannot be loaded. Problem is, the original design
 of @c MPDiskLRUCache uses a SHA1 hash key for the file name of local cache file without the file
 extension, and thus the cache file cannot be loaded into @c AVPlayer directly. This @c MPMediaFileCache
 protocol is designed to solve this problem by preserving the original file extension in the cache
 file. So, for @c AVPlayer relate media file access, use the API in this @c MediaFile category only.
 */
@protocol MPMediaFileCache <NSObject>

/**
 Determine whether a remote media file has been locally cached.
 */
- (BOOL)isRemoteFileCached:(NSURL *)remoteFileURL;

/**
 Move a file to the cache directory.
 @param localFileURL The location of the file to move. Typically this source file is a temporary file
 provided by the completion handler of a URL session download task.
 @param remoteFileURL The original remote URL that the file was hosted.
 */
- (NSError *)moveLocalFileToCache:(NSURL *)localFileURL remoteSourceFileURL:(NSURL *)remoteFileURL;

@optional

/**
 "Touch" (update with current date) @c NSFileModificationDate of the file for LRU tracking or other
 purpose. @c NSFileModificationDate is updated because iOS doesn't provide "last opened date" access.
 */
- (void)touchCachedFileForRemoteFile:(NSURL *)remoteFileURL;

@end

NS_ASSUME_NONNULL_END

#endif /* MPMediaFileCache_h */
