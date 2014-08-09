
#import "ImageDownloader.h"

@implementation ImageDownloader

- (void)startDownloadingWithURL:(NSURL *)URL
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        NSURLRequest * request = [NSURLRequest requestWithURL:URL cachePolicy:NSURLRequestReturnCacheDataElseLoad timeoutInterval:20];
        NSData * data = [NSURLConnection sendSynchronousRequest:request returningResponse:nil error:nil];
        self.image = [UIImage imageWithData:data scale:[UIScreen mainScreen].scale];
        dispatch_sync(dispatch_get_main_queue(), ^{
            [self.delegate imageDownloaderDidFinishLoading:self];
        });
    });
}

@end
