#import "MWMShareActivityItem.h"

#include <CoreApi/Framework.h>

#import <LinkPresentation/LPLinkMetadata.h>

static NSAttributedString * AttributedBodyFromHtml(NSString * html)
{
  NSData * data = [html dataUsingEncoding:NSUTF8StringEncoding];
  if (!data)
    return nil;
  NSDictionary * options = @{
    NSDocumentTypeDocumentAttribute: NSHTMLTextDocumentType,
    NSCharacterEncodingDocumentAttribute: @(NSUTF8StringEncoding)
  };
  return [[NSAttributedString alloc] initWithData:data options:options documentAttributes:nil error:nil];
}

static NSURL * EncodedShareURL(NSString * urlString)
{
  // ge0 %-escapes unsafe ASCII but leaves the place name's non-ASCII bytes raw, and +URLWithString: rejects
  // those before iOS 17. Encoding only non-ASCII cannot double-encode the escapes ge0 already produced.
  // '\' is the single unsafe ASCII byte ge0 leaves raw, so it has to be encoded here as well.
  NSMutableCharacterSet * allowed = [[NSCharacterSet characterSetWithRange:NSMakeRange(0, 128)] mutableCopy];
  [allowed removeCharactersInString:@"\\"];
  NSString * url = [urlString stringByAddingPercentEncodingWithAllowedCharacters:allowed];
  return [NSURL URLWithString:url];
}

@interface MWMTypedShareActivityItem : NSObject <UIActivityItemSource>

- (instancetype)initWithItem:(id)item activityType:(UIActivityType)activityType subject:(NSString *)subject;

@end

@implementation MWMTypedShareActivityItem
{
  id _item;
  UIActivityType _activityType;
  NSString * _subject;
}

- (instancetype)initWithItem:(id)item activityType:(UIActivityType)activityType subject:(NSString *)subject
{
  self = [super init];
  if (self)
  {
    _item = item;
    _activityType = [activityType copy];
    _subject = [subject copy];
  }
  return self;
}

- (id)activityViewControllerPlaceholderItem:(UIActivityViewController *)activityViewController
{
  return _item;
}

- (id)activityViewController:(UIActivityViewController *)activityViewController
         itemForActivityType:(NSString *)activityType
{
  return [_activityType isEqualToString:activityType] ? _item : nil;
}

- (NSString *)activityViewController:(UIActivityViewController *)activityViewController
              subjectForActivityType:(NSString *)activityType
{
  return _subject;
}

@end

@interface MWMShareActivityItem () <UIActivityItemSource>

@property(nonatomic) NSURL * shareURL;
@property(nonatomic, copy) NSString * shareText;
@property(nonatomic, copy) NSAttributedString * attributedBody;
@property(nonatomic, copy) NSString * subject;

@end

@implementation MWMShareActivityItem

- (instancetype)initForMyPositionAtLocation:(CLLocationCoordinate2D const &)location
{
  self = [super init];
  if (self)
    [self fillFrom:GetFramework().GetShareDataForMyPosition(ms::LatLon(location.latitude, location.longitude))
        isMyPosition:YES];
  return self;
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
    [self fillFrom:f.GetShareData(info) isMyPosition:info.IsMyPosition()];
  }
  return self;
}

- (void)fillFrom:(share::Result const &)result isMyPosition:(BOOL)isMyPosition
{
  _shareURL = EncodedShareURL(@(result.m_url.c_str()));
  _shareText = @(result.m_text.c_str());
  _attributedBody = AttributedBodyFromHtml(@(result.m_html.c_str()));

  // Email subject: place name/address, "I am here" for the current position, or a generic fallback.
  if (isMyPosition)
    _subject = L(@"share_my_position");
  else if (!result.m_subjectBasis.empty())
    _subject = [NSString stringWithFormat:L(@"share_place_subject"), @(result.m_subjectBasis.c_str())];
  else
    _subject = L(@"share_place_subject_default");
}

- (NSArray *)activityItems
{
  NSMutableArray * items = [NSMutableArray arrayWithObject:self];
  // Separate sources keep each placeholder class consistent with the item returned for its target activity.
  // dataTypeIdentifierForActivityType: only applies to NSData and can't make an NSString placeholder a URL.
  if (self.attributedBody)
    [items addObject:[[MWMTypedShareActivityItem alloc] initWithItem:self.attributedBody
                                                        activityType:UIActivityTypeMail
                                                             subject:self.subject]];
  if (self.shareURL)
    [items addObject:[[MWMTypedShareActivityItem alloc] initWithItem:self.shareURL
                                                        activityType:UIActivityTypeAirDrop
                                                             subject:self.subject]];
  return items.copy;
}

- (LPLinkMetadata *)activityViewControllerLinkMetadata:(UIActivityViewController *)activityViewController
{
  LPLinkMetadata * metadata = [[LPLinkMetadata alloc] init];
  metadata.originalURL = self.shareURL;
  metadata.title = self.subject;
  metadata.iconProvider = [[NSItemProvider alloc] initWithObject:[UIImage imageNamed:@"imgLogo"]];
  return metadata;
}

#pragma mark - UIActivityItemSource

- (id)activityViewControllerPlaceholderItem:(UIActivityViewController *)activityViewController
{
  return self.shareText;
}

- (id)activityViewController:(UIActivityViewController *)activityViewController
         itemForActivityType:(NSString *)activityType
{
  // The typed Mail and AirDrop sources return their own items for these activities.
  if ((self.attributedBody && [UIActivityTypeMail isEqualToString:activityType]) ||
      (self.shareURL && [UIActivityTypeAirDrop isEqualToString:activityType]))
    return nil;
  return self.shareText;
}

- (NSString *)activityViewController:(UIActivityViewController *)activityViewController
              subjectForActivityType:(NSString *)activityType
{
  return self.subject;
}

@end
