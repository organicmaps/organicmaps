//
//  MPMediaFileCache.h
//
//  Copyright 2018-2020 Twitter, Inc.
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
 Obtain the expected local cache file URL provided the remote file URL.
 Note: The cached file referenced by the returned URL may not exist. After the remote data is
 downloaded, use `storeData:forRemoteSourceFileURL:` to store it to the returned cache file URL.
 */
- (NSURL *)cachedFileURLForRemoteFileURL:(NSURL *)remoteFileURL;

/**
 Determine whether a remote media file has been locally cached.
 */
- (BOOL)isRemoteFileCached:(NSURL *)remoteFileURL;

/**
 Store data to the cache directory.
 @param data The data to write.
 @param remoteFileURL The original remote URL that the file was hosted.
 */
- (void)storeData:(NSData *)data forRemoteSourceFileURL:(NSURL *)remoteFileURL;

@optional

/**
 "Touch" (update with current date) @c NSFileModificationDate of the file for LRU tracking or other
 purpose. @c NSFileModificationDate is updated because iOS doesn't provide "last opened date" access.
 */
- (void)touchCachedFileForRemoteFile:(NSURL *)remoteFileURL;

@end

NS_ASSUME_NONNULL_END

#endif /* MPMediaFileCache_h */
