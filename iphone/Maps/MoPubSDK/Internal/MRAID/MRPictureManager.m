//
// Copyright (c) 2013 MoPub. All rights reserved.
//

#import "MRPictureManager.h"
#import "MPInstanceProvider.h"

@interface MRPictureManager ()

@property (nonatomic, retain) MRImageDownloader *imageDownloader;
@property (nonatomic, copy) NSURL *imageURL;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MRPictureManager

@synthesize delegate = _delegate;
@synthesize imageDownloader = _imageDownloader;
@synthesize imageURL = _imageURL;

- (id)initWithDelegate:(id<MRPictureManagerDelegate>)delegate
{
    if (self = [super init]) {
        _delegate = delegate;
    }
    return self;
}

- (void)dealloc
{
    [_imageDownloader setDelegate:nil];
    [_imageDownloader release];
    [_imageURL release];
    [super dealloc];
}

- (void)storePicture:(NSDictionary *)parameters
{
    self.imageURL = [NSURL URLWithString:[[parameters objectForKey:@"uri"] stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];

    if (!self.imageURL) {
        [self.delegate pictureManager:self
                       didFailToStorePictureWithErrorMessage:@"URI was not valid."];
        return;
    }

    UIAlertView *alertView = [[[UIAlertView alloc] initWithTitle:nil
                                                         message:@"Save this image to your photos?"
                                                        delegate:self
                                               cancelButtonTitle:@"Cancel"
                                               otherButtonTitles:@"Save Image", nil] autorelease];
    [alertView show];
}

#pragma mark - <UIAlertViewDelegate>

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (buttonIndex == alertView.cancelButtonIndex) {
        [self.delegate pictureManager:self
                       didFailToStorePictureWithErrorMessage:@"The user canceled the action."];
    } else if (buttonIndex == alertView.firstOtherButtonIndex) {
        if (!self.imageDownloader) {
            self.imageDownloader = [[MPInstanceProvider sharedProvider] buildMRImageDownloaderWithDelegate:self];
        }

        [self.imageDownloader downloadImageWithURL:self.imageURL];
    }
}

#pragma mark - MRImageDownloader Delegate

- (void)downloaderDidFailToSaveImageWithURL:(NSURL *)URL error:(NSError *)error
{
    [self.delegate pictureManager:self
                   didFailToStorePictureWithErrorMessage:@"Picture could not be saved."];
}

@end
