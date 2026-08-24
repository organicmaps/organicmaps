#import "MWMShareActivityItem.h"

#include "base/assert.hpp"

#include <CoreApi/Framework.h>

#import <LinkPresentation/LPLinkMetadata.h>

@interface MWMAirDropActivityItem : NSObject <UIActivityItemSource>

- (instancetype)initWithURL:(NSURL *)url;

@end

@implementation MWMAirDropActivityItem
{
  NSURL * _url;
}

- (instancetype)initWithURL:(NSURL *)url
{
  self = [super init];
  if (self)
    _url = url;
  return self;
}

- (id)activityViewControllerPlaceholderItem:(UIActivityViewController *)activityViewController
{
  return _url;
}

- (id)activityViewController:(UIActivityViewController *)activityViewController
         itemForActivityType:(UIActivityType)activityType
{
  return [activityType isEqualToString:UIActivityTypeAirDrop] ? _url : nil;
}

@end

@interface MWMShareActivityItem () <UIActivityItemSource>

@property(nonatomic) BOOL isMyPosition;
@property(nonatomic) NSURL * shareUrl;
@property(nonatomic, copy) NSString * shareText;
@property(nonatomic, copy) NSString * shareHtml;
@property(nonatomic, copy) NSString * subjectBasis;

@end

@implementation MWMShareActivityItem

- (instancetype)initForMyPositionAtLocation:(CLLocationCoordinate2D const &)location
{
  self = [super init];
  if (self)
    [self fillFrom:GetFramework().GetShareDataForMyPosition(ms::LatLon(location.latitude, location.longitude))];
  return self;
}

- (instancetype)initForCurrentPlacePage
{
  self = [super init];
  if (self)
  {
    auto & f = GetFramework();
    [self fillFrom:f.GetShareData(f.GetCurrentPlacePageInfo())];
  }
  return self;
}

- (void)fillFrom:(share::Result const &)result
{
  _isMyPosition = result.m_isMyPosition;
  // ge0 %-escapes unsafe ASCII but leaves non-ASCII place name bytes raw, which NSURL rejects before iOS 17.
  // Encoding only non-ASCII preserves existing ge0 escapes. Backslash is the only unsafe ASCII byte ge0 leaves raw,
  // so it is encoded here as well.
  NSMutableCharacterSet * allowed = [[NSCharacterSet characterSetWithRange:NSMakeRange(0, 128)] mutableCopy];
  [allowed removeCharactersInString:@"\\"];
  NSString * url = [@(result.m_url.c_str()) stringByAddingPercentEncodingWithAllowedCharacters:allowed];
  _shareUrl = [NSURL URLWithString:url];
  ASSERT(_shareUrl != nil, ("Failed to build a share URL from", result.m_url));
  _shareText = @(result.m_text.c_str());
  _shareHtml = @(result.m_html.c_str());
  _subjectBasis = @(result.m_subjectBasis.c_str());
}

- (NSArray<id<UIActivityItemSource>> *)activityItems
{
  // AirDrop types its payload from the source's placeholder, and this source's placeholder is plain text.
  // A separate NSURL-placeholder source provides an openable link only to AirDrop; dataTypeIdentifierForActivityType:
  // applies to NSData only. A bare NSURL item would go to every target and duplicate the link in shareText.
  return @[self, [[MWMAirDropActivityItem alloc] initWithURL:self.shareUrl]];
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
// This is created lazily after Mail is selected so other share extensions only receive plain text.
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
  return self.shareText;
}

- (id)activityViewController:(UIActivityViewController *)activityViewController
         itemForActivityType:(UIActivityType)activityType
{
  // The URL-typed source in -activityItems serves AirDrop; returning text here would add a second payload.
  if ([activityType isEqualToString:UIActivityTypeAirDrop])
    return nil;
  if ([activityType isEqualToString:UIActivityTypeMail])
    return [self attributedBody] ?: [[NSAttributedString alloc] initWithString:self.shareText];
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
  metadata.originalURL = self.shareUrl;
  metadata.title = [self subject];
  metadata.iconProvider = [[NSItemProvider alloc] initWithObject:[UIImage imageNamed:@"imgLogo"]];
  return metadata;
}

@end
