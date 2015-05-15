//
//  MWMWatchNotification.m
//  Maps
//
//  Created by i.grechuhin on 10.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMWatchNotification.h"
#import "Common.h"

#include <CoreFoundation/CoreFoundation.h>

static NSString * const kMWMWatchNotificationName = @"MWMWatchNotificationName";
static NSString * const kMWMWatchNotificationPath = @"MWMWatchNotificationPath";
static NSString * const kMWMWatchNotificationIdentifier = @"MWMWatchNotificationIdentifier";
static NSString * const kMWMWatchNotificationArchiveNameFormat = @"%@.archive";

@interface MWMWatchNotification ()

@property (strong, nonatomic) NSFileManager *fileManager;
@property (strong, nonatomic) NSMutableDictionary *listenerBlocks;

@end

@implementation MWMWatchNotification

- (instancetype)init
{
  if (![[NSFileManager defaultManager] respondsToSelector:@selector(containerURLForSecurityApplicationGroupIdentifier:)])
    return nil;

  self = [super init];
  if (self)
  {
    self.fileManager = [[NSFileManager alloc] init];
    self.listenerBlocks = [NSMutableDictionary dictionary];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(didReceiveMessageNotification:) name:kMWMWatchNotificationName object:nil];
  }
  return self;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  CFNotificationCenterRef const center = CFNotificationCenterGetDarwinNotifyCenter();
  CFNotificationCenterRemoveEveryObserver(center, (__bridge const void *)(self));
}


#pragma mark - Private File Operation Methods

- (NSString *)messagePassingDirectoryPath
{
  NSURL *appGroupContainer = [self.fileManager containerURLForSecurityApplicationGroupIdentifier:kApplicationGroupIdentifier()];
  NSString *appGroupContainerPath = [appGroupContainer path];
  NSString *directoryPath = [appGroupContainerPath stringByAppendingPathComponent:kMWMWatchNotificationPath];

  [self.fileManager createDirectoryAtPath:directoryPath withIntermediateDirectories:YES attributes:nil error:nil];

  return directoryPath;
}

- (NSString *)filePathForIdentifier:(NSString *)identifier
{
  if (identifier && identifier.length > 0)
  {
    NSString *directoryPath = [self messagePassingDirectoryPath];
    NSString *fileName = [NSString stringWithFormat:kMWMWatchNotificationArchiveNameFormat, identifier];
    NSString *filePath = [directoryPath stringByAppendingPathComponent:fileName];

    return filePath;
  }
  return nil;
}

- (void)writeMessageObject:(id)messageObject toFileWithIdentifier:(NSString *)identifier
{
  if (identifier) {
    if (messageObject)
    {
      NSData *data = [NSKeyedArchiver archivedDataWithRootObject:messageObject];
      NSString *filePath = [self filePathForIdentifier:identifier];

      if (data == nil || filePath == nil || ![data writeToFile:filePath atomically:YES])
        return;

    }

    [self sendNotificationForMessageWithIdentifier:identifier];
  }
}

- (void)deleteFileForIdentifier:(NSString *)identifier
{
  [self.fileManager removeItemAtPath:[self filePathForIdentifier:identifier] error:nil];
}

#pragma mark - Private Notification Methods

- (void)sendNotificationForMessageWithIdentifier:(NSString *)identifier
{
  CFNotificationCenterRef const center = CFNotificationCenterGetDarwinNotifyCenter();
  CFDictionaryRef const userInfo = NULL;
  BOOL const deliverImmediately = YES;
  CFStringRef str = (__bridge CFStringRef)identifier;
  CFNotificationCenterPostNotification(center, str, NULL, userInfo, deliverImmediately);
}

- (void)registerForNotificationsWithIdentifier:(NSString *)identifier
{
  CFNotificationCenterRef const center = CFNotificationCenterGetDarwinNotifyCenter();
  CFStringRef str = (__bridge CFStringRef)identifier;
  CFNotificationCenterAddObserver(center, (__bridge const void *)(self), MWMWatchNotificationCallback, str, NULL, CFNotificationSuspensionBehaviorDeliverImmediately);
}

- (void)unregisterForNotificationsWithIdentifier:(NSString *)identifier
{
  CFNotificationCenterRef const center = CFNotificationCenterGetDarwinNotifyCenter();
  CFStringRef str = (__bridge CFStringRef)identifier;
  CFNotificationCenterRemoveObserver(center, (__bridge const void *)(self), str, NULL);
}

void MWMWatchNotificationCallback(CFNotificationCenterRef center, void * observer, CFStringRef name, void const * object, CFDictionaryRef userInfo)
{
  NSString *identifier = (__bridge NSString *)name;
  [[NSNotificationCenter defaultCenter] postNotificationName:kMWMWatchNotificationName object:nil userInfo:@{kMWMWatchNotificationIdentifier : identifier}];
}

- (void)didReceiveMessageNotification:(NSNotification *)notification
{
  typedef void (^MessageListenerBlock)(id messageObject);

  NSDictionary *userInfo = notification.userInfo;
  NSString *identifier = [userInfo valueForKey:kMWMWatchNotificationIdentifier];

  if (identifier)
  {
    MessageListenerBlock listenerBlock = [self listenerBlockForIdentifier:identifier];

    if (listenerBlock)
    {
      id messageObject = [self messageWithIdentifier:identifier];

      listenerBlock(messageObject);
    }
  }
}

- (id)listenerBlockForIdentifier:(NSString *)identifier
{
  return [self.listenerBlocks valueForKey:identifier];
}

#pragma mark - Public Interface Methods

- (void)passMessageObject:(id <NSCoding>)messageObject identifier:(NSString *)identifier
{
  [self writeMessageObject:messageObject toFileWithIdentifier:identifier];
}

- (id)messageWithIdentifier:(NSString *)identifier
{
  if (identifier)
  {
    NSData *data = [NSData dataWithContentsOfFile:[self filePathForIdentifier:identifier]];

    if (data)
      return [NSKeyedUnarchiver unarchiveObjectWithData:data];
  }
  return nil;
}

- (void)clearMessageContentsForIdentifier:(NSString *)identifier
{
  [self deleteFileForIdentifier:identifier];
}

- (void)clearAllMessageContents
{
  NSString *directoryPath = [self messagePassingDirectoryPath];
  NSArray *messageFiles = [self.fileManager contentsOfDirectoryAtPath:directoryPath error:nil];
  [messageFiles enumerateObjectsUsingBlock:^(NSString *path, NSUInteger idx, BOOL *stop)
  {
    NSString *filePath = [directoryPath stringByAppendingPathComponent:path];
    [self.fileManager removeItemAtPath:filePath error:nil];
  }];
}

- (void)listenForMessageWithIdentifier:(NSString *)identifier listener:(void (^)(id messageObject))listener
{
  if (identifier)
  {
    [self.listenerBlocks setValue:listener forKey:identifier];
    [self registerForNotificationsWithIdentifier:identifier];
  }
}

- (void)stopListeningForMessageWithIdentifier:(NSString *)identifier
{
  if (identifier)
  {
    [self.listenerBlocks setValue:nil forKey:identifier];
    [self unregisterForNotificationsWithIdentifier:identifier];
  }
}

@end
