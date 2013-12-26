//
// Copyright (c) 2013 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol MRImageDownloaderDelegate;

@interface MRImageDownloader : NSObject

@property (nonatomic, assign) id<MRImageDownloaderDelegate> delegate;
@property (nonatomic, retain) NSOperationQueue *queue;
@property (nonatomic, retain) NSMutableDictionary *pendingOperations;

- (id)initWithDelegate:(id<MRImageDownloaderDelegate>)delegate;
- (void)downloadImageWithURL:(NSURL *)URL;

@end

@protocol MRImageDownloaderDelegate <NSObject>

@required
- (void)downloaderDidFailToSaveImageWithURL:(NSURL *)URL error:(NSError *)error;

@end
