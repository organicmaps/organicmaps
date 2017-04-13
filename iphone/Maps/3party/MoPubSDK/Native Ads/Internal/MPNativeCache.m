//
//  MPNativeCache.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPNativeCache.h"
#import "MPDiskLRUCache.h"
#import "MPLogging.h"

typedef enum {
    MPNativeCacheMethodDisk = 0,
    MPNativeCacheMethodDiskAndMemory = 1 << 0
} MPNativeCacheMethod;

@interface MPNativeCache () <NSCacheDelegate>

@property (nonatomic, strong) NSCache *memoryCache;
@property (nonatomic, strong) MPDiskLRUCache *diskCache;
@property (nonatomic, assign) MPNativeCacheMethod cacheMethod;

- (BOOL)cachedDataExistsForKey:(NSString *)key withCacheMethod:(MPNativeCacheMethod)cacheMethod;
- (NSData *)retrieveDataForKey:(NSString *)key withCacheMethod:(MPNativeCacheMethod)cacheMethod;
- (void)storeData:(id)data forKey:(NSString *)key withCacheMethod:(MPNativeCacheMethod)cacheMethod;
- (void)removeAllDataFromMemory;
- (void)removeAllDataFromDisk;

@end

@implementation MPNativeCache

+ (instancetype)sharedCache;
{
    static dispatch_once_t once;
    static MPNativeCache *sharedCache;
    dispatch_once(&once, ^{
        sharedCache = [[self alloc] init];
    });
    return sharedCache;
}

- (id)init
{
    self = [super init];
    if (self != nil) {
        _memoryCache = [[NSCache alloc] init];
        _memoryCache.delegate = self;

        _diskCache = [[MPDiskLRUCache alloc] init];

        _cacheMethod = MPNativeCacheMethodDiskAndMemory;

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(didReceiveMemoryWarning:) name:UIApplicationDidReceiveMemoryWarningNotification object:[UIApplication sharedApplication]];
    }

    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self name:UIApplicationDidReceiveMemoryWarningNotification object:nil];
}

#pragma mark - Public Cache Interactions

- (void)setInMemoryCacheEnabled:(BOOL)enabled
{
    if (enabled) {
        self.cacheMethod = MPNativeCacheMethodDiskAndMemory;
    } else {
        self.cacheMethod = MPNativeCacheMethodDisk;
        [self.memoryCache removeAllObjects];
    }
}

- (BOOL)cachedDataExistsForKey:(NSString *)key
{
    return [self cachedDataExistsForKey:key withCacheMethod:self.cacheMethod];
}

- (NSData *)retrieveDataForKey:(NSString *)key
{
    return [self retrieveDataForKey:key withCacheMethod:self.cacheMethod];
}

- (void)storeData:(NSData *)data forKey:(NSString *)key
{
    [self storeData:data forKey:key withCacheMethod:self.cacheMethod];
}

- (void)removeAllDataFromCache
{
    [self removeAllDataFromMemory];
    [self removeAllDataFromDisk];
}

#pragma mark - Private Cache Implementation

- (BOOL)cachedDataExistsForKey:(NSString *)key withCacheMethod:(MPNativeCacheMethod)cacheMethod
{
    BOOL dataExists = NO;
    if (cacheMethod & MPNativeCacheMethodDiskAndMemory) {
        dataExists = [self.memoryCache objectForKey:key] != nil;
    }

    if (!dataExists) {
        dataExists = [self.diskCache cachedDataExistsForKey:key];
    }

    return dataExists;
}

- (id)retrieveDataForKey:(NSString *)key withCacheMethod:(MPNativeCacheMethod)cacheMethod
{
    id data = nil;

    if (cacheMethod & MPNativeCacheMethodDiskAndMemory) {
        data = [self.memoryCache objectForKey:key];
    }

    if (data) {
        MPLogDebug(@"RETRIEVE FROM MEMORY: %@", key);
    }


    if (data == nil) {
        data = [self.diskCache retrieveDataForKey:key];

        if (data && cacheMethod & MPNativeCacheMethodDiskAndMemory) {
            MPLogDebug(@"RETRIEVE FROM DISK: %@", key);

            [self.memoryCache setObject:data forKey:key];
            MPLogDebug(@"STORED IN MEMORY: %@", key);
        }
    }

    if (data == nil) {
        MPLogDebug(@"RETRIEVE FAILED: %@", key);
    }

    return data;
}

- (void)storeData:(id)data forKey:(NSString *)key withCacheMethod:(MPNativeCacheMethod)cacheMethod
{
    if (data == nil) {
        return;
    }

    if (cacheMethod & MPNativeCacheMethodDiskAndMemory) {
        [self.memoryCache setObject:data forKey:key];
        MPLogDebug(@"STORED IN MEMORY: %@", key);
    }

    [self.diskCache storeData:data forKey:key];
    MPLogDebug(@"STORED ON DISK: %@", key);
}

- (void)removeAllDataFromMemory
{
    [self.memoryCache removeAllObjects];
}

- (void)removeAllDataFromDisk
{
    [self.diskCache removeAllCachedFiles];
}

#pragma mark - Notifications

- (void)didReceiveMemoryWarning:(NSNotification *)notification
{
    [self.memoryCache removeAllObjects];
}

#pragma mark - NSCacheDelegate

- (void)cache:(NSCache *)cache willEvictObject:(id)obj
{
    MPLogDebug(@"Evicting Object");
}


@end
