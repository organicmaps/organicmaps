#import "MWMFileShareActivityItem.h"

#import <LinkPresentation/LPLinkMetadata.h>

@interface MWMFileShareActivityItem ()

@property(nonatomic) NSURL * fileURL;
@property(nonatomic, copy) NSString * displayName;

@end

@implementation MWMFileShareActivityItem

- (instancetype)initWithFileURL:(NSURL *)url displayName:(NSString *)displayName
{
  self = [super init];
  if (self)
  {
    _fileURL = url;
    _displayName = [displayName copy];
  }
  return self;
}

#pragma mark - UIActivityItemSource

- (id)activityViewControllerPlaceholderItem:(UIActivityViewController *)activityViewController
{
  return self.fileURL;
}

- (id)activityViewController:(UIActivityViewController *)activityViewController
         itemForActivityType:(NSString *)activityType
{
  NSItemProvider * provider = [[NSItemProvider alloc] initWithContentsOfURL:self.fileURL];
  if (provider)
  {
    NSString * ext = self.fileURL.pathExtension;
    NSString * name = self.displayName;
    // Avoid double extension (e.g. "Track.gpx.gpx").
    if (ext.length > 0 && ![name.pathExtension isEqualToString:ext])
      name = [NSString stringWithFormat:@"%@.%@", name, ext];
    provider.suggestedName = name;
    return provider;
  }
  return self.fileURL;
}

- (NSString *)activityViewController:(UIActivityViewController *)activityViewController
              subjectForActivityType:(NSString *)activityType
{
  return self.displayName;
}

- (LPLinkMetadata *)activityViewControllerLinkMetadata:(UIActivityViewController *)activityViewController
{
  LPLinkMetadata * metadata = [[LPLinkMetadata alloc] init];
  metadata.originalURL = self.fileURL;
  metadata.title = self.displayName;
  metadata.iconProvider = [[NSItemProvider alloc] initWithObject:[UIImage imageNamed:@"imgLogo"]];
  return metadata;
}

@end
