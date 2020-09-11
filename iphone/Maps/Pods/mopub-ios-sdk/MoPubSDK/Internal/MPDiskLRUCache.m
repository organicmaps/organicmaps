//
//  MPDiskLRUCache.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPDiskLRUCache.h"
#import "MPGlobal.h"
#import "MPLogging.h"

#import <CommonCrypto/CommonDigest.h>

// cached files that have not been access since kCacheFileMaxAge ago will be evicted
#define kCacheFileMaxAge (7 * 24 * 60 * 60) // 1 week

// once the cache hits this size AND we've added at least kCacheBytesStoredBeforeSizeCheck bytes,
// cached files will be evicted (LRU) until the total size drops below this limit
#define kCacheSoftMaxSize (100 * 1024 * 1024) // 100 MB

#define kCacheBytesStoredBeforeSizeCheck (kCacheSoftMaxSize / 10) // 10% of kCacheSoftMaxSize

@interface MPDiskLRUCacheFile : NSObject

@property (nonatomic, copy) NSString *filePath;
@property (nonatomic, assign) NSTimeInterval lastModTimestamp;
@property (nonatomic, assign) uint64_t fileSize;

@end

@implementation MPDiskLRUCacheFile
@end // this data object should have empty implementation

@interface MPDiskLRUCache ()

/**
 Note: Only use this @c diskIOQueue for direct operations to fileManager, and avoid nested access
 to @c diskIOQueue to avoid crash.
 */
@property (nonatomic, strong) dispatch_queue_t diskIOQueue;
@property (nonatomic, strong) NSFileManager *fileManager;
@property (nonatomic, copy) NSString *diskCachePath;
@property (atomic, assign) uint64_t numBytesStoredForSizeCheck;

@end

@implementation MPDiskLRUCache

+ (MPDiskLRUCache *)sharedDiskCache
{
    static dispatch_once_t once;
    static MPDiskLRUCache *sharedDiskCache;
    dispatch_once(&once, ^{
        sharedDiskCache = [self new];
    });
    return sharedDiskCache;
}

- (id)init
{
    return [self initWithCachePath:@"com.mopub.diskCache"
                       fileManager:[NSFileManager defaultManager]];
}

#pragma mark Private

- (id)initWithCachePath:(NSString *)cachePath fileManager:(NSFileManager *)fileManager {
    self = [super init];
    if (self != nil) {
        _diskIOQueue = dispatch_queue_create("com.mopub.diskCacheIOQueue", DISPATCH_QUEUE_SERIAL);

        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
        if (paths.count > 0) {
            _diskCachePath = [[paths objectAtIndex:0] stringByAppendingPathComponent:cachePath];
            _fileManager = fileManager;

            if (![_fileManager fileExistsAtPath:_diskCachePath]) {
                [_fileManager createDirectoryAtPath:_diskCachePath
                        withIntermediateDirectories:YES
                                         attributes:nil
                                              error:nil];
            }
        }

        // check cache size on startup
        [self ensureCacheSizeLimit];
    }

    return self;
}

- (void)ensureCacheSizeLimit
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        MPLogDebug(@"Checking cache size...");

        NSMutableArray *cacheFilesSortedByModDate = [self cacheFilesSortedByModDate];

        dispatch_async(self.diskIOQueue, ^{
            @autoreleasepool {
                // verify age
                NSArray *expiredFiles = [self expiredCachedFilesInArray:cacheFilesSortedByModDate];
                for (MPDiskLRUCacheFile *file in expiredFiles) {
                    MPLogDebug(@"Trying to remove %@ from cache due to expiration", file.filePath);

                    [self.fileManager removeItemAtPath:file.filePath error:nil];
                    [cacheFilesSortedByModDate removeObject:file];
                }

                // verify size
                while ([self sizeOfCacheFilesInArray:cacheFilesSortedByModDate] >= kCacheSoftMaxSize && cacheFilesSortedByModDate.count > 0) {
                    NSString *oldestFilePath = ((MPDiskLRUCacheFile *)[cacheFilesSortedByModDate objectAtIndex:0]).filePath;

                    MPLogDebug(@"Trying to remove %@ from cache due to size", oldestFilePath);

                    [self.fileManager removeItemAtPath:oldestFilePath error:nil];
                    [cacheFilesSortedByModDate removeObjectAtIndex:0];
                }
            }
        });
    });
}

- (NSArray *)expiredCachedFilesInArray:(NSArray *)cachedFiles
{
    NSMutableArray *result = [NSMutableArray array];

    NSTimeInterval now = [[NSDate date] timeIntervalSinceReferenceDate];

    for (MPDiskLRUCacheFile *file in cachedFiles) {
        if (now - file.lastModTimestamp >= kCacheFileMaxAge) {
            [result addObject:file];
        }
    }

    return result;
}

- (NSMutableArray *)cacheFilesSortedByModDate
{
    NSArray *cachedFiles = [self.fileManager contentsOfDirectoryAtPath:self.diskCachePath error:nil];
    NSArray *sortedFiles = [cachedFiles sortedArrayUsingComparator:^NSComparisonResult(id obj1, id obj2) {
        NSString *fileName1 = [self.diskCachePath stringByAppendingPathComponent:(NSString *)obj1];
        NSString *fileName2 = [self.diskCachePath stringByAppendingPathComponent:(NSString *)obj2];

        NSDictionary *fileAttrs1 = [self.fileManager attributesOfItemAtPath:fileName1 error:nil];
        NSDictionary *fileAttrs2 = [self.fileManager attributesOfItemAtPath:fileName2 error:nil];

        NSDate *lastModDate1 = [fileAttrs1 fileModificationDate];
        NSDate *lastModDate2 = [fileAttrs2 fileModificationDate];

        return [lastModDate1 compare:lastModDate2];
    }];

    NSMutableArray *result = [NSMutableArray array];

    for (NSString *fileName in sortedFiles) {
        if ([fileName hasPrefix:@"."]) {
            continue;
        }

        MPDiskLRUCacheFile *cacheFile = [[MPDiskLRUCacheFile alloc] init];
        cacheFile.filePath = [self.diskCachePath stringByAppendingPathComponent:fileName];

        NSDictionary *fileAttrs = [self.fileManager attributesOfItemAtPath:cacheFile.filePath error:nil];
        cacheFile.fileSize = [fileAttrs fileSize];
        cacheFile.lastModTimestamp = [[fileAttrs fileModificationDate] timeIntervalSinceReferenceDate];

        [result addObject:cacheFile];
    }

    return result;
}

- (uint64_t)sizeOfCacheFilesInArray:(NSArray *)files
{
    uint64_t currentSize = 0;

    for (MPDiskLRUCacheFile *file in files) {
        currentSize += file.fileSize;
    }

    MPLogDebug(@"Current cache size %qu bytes", currentSize);

    return currentSize;
}

- (NSString *)cacheFilePathForKey:(NSString *)key
{
    NSString *hashedKey = MPSHA1Digest(key);
    NSString *cachedFilePath = [self.diskCachePath stringByAppendingPathComponent:hashedKey];
    return cachedFilePath;
}

/**
 "touch" @c NSFileModificationDate of the file for LRU tracking. @c NSFileModificationDate is used
 because iOS does not provide a "last access date".
 */
- (void)touchCacheFileAtPath:(NSString *)cachedFilePath
{
    dispatch_sync(self.diskIOQueue, ^{
        [self.fileManager setAttributes:@{NSFileModificationDate: [NSDate date]}
                           ofItemAtPath:cachedFilePath
                                  error:nil];
    });
}

@end

#pragma mark - MPDiskLRUCache

@implementation MPDiskLRUCache (MPDiskLRUCache)

- (BOOL)cachedDataExistsForKey:(NSString *)key
{
    __block BOOL result = NO;

    dispatch_sync(self.diskIOQueue, ^{
        result = [self.fileManager fileExistsAtPath:[self cacheFilePathForKey:key]];
    });

    return result;
}

- (NSData *)retrieveDataForKey:(NSString *)key
{
    __block NSData *data = nil;

    if ([self cachedDataExistsForKey:key]) {
        NSString *cacheFilePath = [self cacheFilePathForKey:key];
        data = [NSData dataWithContentsOfFile:cacheFilePath];
        [self touchCacheFileAtPath:cacheFilePath];
    }

    return data;
}

- (void)storeData:(NSData *)data forKey:(NSString *)key
{
    NSString *cacheFilePath = [self cacheFilePathForKey:key];

    dispatch_sync(self.diskIOQueue, ^{
        if (![self.fileManager fileExistsAtPath:cacheFilePath]) {
            [self.fileManager createFileAtPath:cacheFilePath contents:data attributes:nil];
        } else {
            // overwrite existing file
            [data writeToFile:cacheFilePath atomically:YES];
        }
    });

    self.numBytesStoredForSizeCheck += data.length;

    if (self.numBytesStoredForSizeCheck >= kCacheBytesStoredBeforeSizeCheck) {
        [self ensureCacheSizeLimit];
        self.numBytesStoredForSizeCheck = 0;
    }
}

- (void)removeAllCachedFiles
{
    dispatch_sync(self.diskIOQueue, ^{
        NSArray *allFiles = [self cacheFilesSortedByModDate];
        for (MPDiskLRUCacheFile *file in allFiles) {
            [self.fileManager removeItemAtPath:file.filePath error:nil];
        }
    });
}

@end

#pragma mark - MPMediaFileCache

@implementation MPDiskLRUCache (MPMediaFileCache)

- (NSString *)cacheKeyForRemoteMediaURL:(NSURL *)remoteFileURL {
    return remoteFileURL.absoluteString;
}

/**
 Obtain the expected local cache file URL provided the remote file URL.
 Note: The cached file referenced by the returned URL may not exist. After the remote data is
 downloaded, use `storeData:forRemoteSourceFileURL:` to store it to the returned cache file URL.
 */
- (NSURL *)cachedFileURLForRemoteFileURL:(NSURL *)remoteFileURL {
    NSString *cacheKey = [self cacheKeyForRemoteMediaURL:remoteFileURL];
    NSURL *pathWithoutExtension = [NSURL fileURLWithPath:[self cacheFilePathForKey:cacheKey]];
    return [pathWithoutExtension URLByAppendingPathExtension:remoteFileURL.pathExtension];
}

- (BOOL)isRemoteFileCached:(NSURL *)remoteFileURL {
    __block BOOL result = NO;
    NSURL *localCacheFileURL = [self cachedFileURLForRemoteFileURL:remoteFileURL];
    dispatch_sync(self.diskIOQueue, ^{
        result = [self.fileManager fileExistsAtPath:localCacheFileURL.path];
    });
    return result;
}

- (void)storeData:(NSData *)data forRemoteSourceFileURL:(NSURL *)remoteFileURL {
    dispatch_sync(self.diskIOQueue, ^{
        [data writeToURL:[self cachedFileURLForRemoteFileURL:remoteFileURL] atomically:YES];
    });
}

- (void)touchCachedFileForRemoteFile:(NSURL *)remoteFileURL {
    NSString *cacheKey = [self cacheKeyForRemoteMediaURL:remoteFileURL];
    NSURL *localCacheFileURL = [self cachedFileURLForRemoteFileURL:remoteFileURL];
    if ([self cachedDataExistsForKey:cacheKey]) {
        [self touchCacheFileAtPath:localCacheFileURL.path];
    }
}

@end
