#import "MWMShareActivityItem.h"

#include <CoreApi/Framework.h>

#import <LinkPresentation/LPLinkMetadata.h>

@interface MWMShareActivityItem ()

@property(nonatomic) BOOL isMyPosition;
@property(nonatomic, copy) NSString * shareUrl;
@property(nonatomic, copy) NSString * shareText;
@property(nonatomic, copy) NSString * shareHtml;
@property(nonatomic, copy) NSString * subjectBasis;

@end

@implementation MWMShareActivityItem

- (instancetype)initForMyPositionAtLocation:(CLLocationCoordinate2D const &)location
{
  self = [super init];
  if (self)
  {
    _isMyPosition = YES;
    [self fillFrom:GetFramework().GetShareDataForMyPosition(ms::LatLon(location.latitude, location.longitude))];
  }
  return self;
}

- (instancetype)initForPlacePageObject:(id<MWMPlacePageObject>)object
{
  NSAssert(false, @"deprecated");

  return nil;
}

- (instancetype)initForPlacePage:(PlacePageData *)data
{
  self = [super init];
  if (self)
  {
    NSAssert(data, @"Entity can't be nil!");
    // The place page is open, so the core has the info (with metadata) to build the shared text.
    auto & f = GetFramework();
    auto const & info = f.GetCurrentPlacePageInfo();
    _isMyPosition = info.IsMyPosition();
    [self fillFrom:f.GetShareData(info)];
  }
  return self;
}

- (void)fillFrom:(share::Result const &)result
{
  _shareUrl = @(result.m_url.c_str());
  _shareText = @(result.m_text.c_str());
  _shareHtml = @(result.m_html.c_str());
  _subjectBasis = @(result.m_subjectBasis.c_str());
}

// Email subject: place name/address, "I am here" for the current position, or a generic fallback.
- (NSString *)subject
{
  if (self.isMyPosition)
    return L(@"share_my_position");
  if (self.subjectBasis.length > 0)
    return [NSString stringWithFormat:L(@"share_place_subject"), self.subjectBasis];
  return L(@"share_place_subject_default");
}

// A rich attributed body so Mail sends formatted HTML; nil when the HTML can't be parsed.
- (NSAttributedString *)attributedBody
{
  NSData * data = [self.shareHtml dataUsingEncoding:NSUTF8StringEncoding];
  if (!data)
    return nil;
  NSDictionary * options = @{
    NSDocumentTypeDocumentAttribute: NSHTMLTextDocumentType,
    NSCharacterEncodingDocumentAttribute: @(NSUTF8StringEncoding)
  };
  return [[NSAttributedString alloc] initWithData:data options:options documentAttributes:nil error:nil];
}

#pragma mark - UIActivityItemSource

- (id)activityViewControllerPlaceholderItem:(UIActivityViewController *)activityViewController
{
  return self.shareUrl;
}

- (id)activityViewController:(UIActivityViewController *)activityViewController
         itemForActivityType:(UIActivityType)activityType
{
  if ([activityType isEqualToString:UIActivityTypeMail])
  {
    if (NSAttributedString * body = [self attributedBody])
      return body;
  }
  return self.shareText;
}

- (NSString *)activityViewController:(UIActivityViewController *)activityViewController
              subjectForActivityType:(UIActivityType)activityType
{
  return [self subject];
}

- (LPLinkMetadata *)activityViewControllerLinkMetadata:(UIActivityViewController *)activityViewController
{
  LPLinkMetadata * metadata = [[LPLinkMetadata alloc] init];
  metadata.originalURL = [NSURL URLWithString:self.shareUrl];
  metadata.title = [self subject];
  metadata.iconProvider = [[NSItemProvider alloc] initWithObject:[UIImage imageNamed:@"imgLogo"]];
  return metadata;
}

@end
