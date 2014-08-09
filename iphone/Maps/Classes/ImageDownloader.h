
#import <Foundation/Foundation.h>

@class ImageDownloader;
@protocol ImageDownloaderDelegate <NSObject>

- (void)imageDownloaderDidFinishLoading:(ImageDownloader *)downloader;

@end

@interface ImageDownloader : NSObject

@property (nonatomic, weak) id <ImageDownloaderDelegate> delegate;
@property (nonatomic) UIImage * image;
@property (nonatomic) NSString * objectId;

- (void)startDownloadingWithURL:(NSURL *)URL;

@end
