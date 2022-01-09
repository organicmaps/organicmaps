#import "MWMShareActivityItem.h"


#include <CoreApi/Framework.h>
#import <CoreApi/PlacePageData.h>
#import <CoreApi/PlacePagePreviewData.h>
#import <CoreApi/PlacePageInfoData.h>

NSString * httpGe0Url(NSString * shortUrl)
{
  // Replace 'om://' with 'https://omaps.app/'
  return [shortUrl stringByReplacingCharactersInRange:NSMakeRange(0, 5) withString:@"https://omaps.app/"];
}

@interface MWMShareActivityItem ()

@property(nonatomic) PlacePageData *data;
@property(nonatomic) CLLocationCoordinate2D location;
@property(nonatomic) BOOL isMyPosition;

@end

@implementation MWMShareActivityItem

- (instancetype)initForMyPositionAtLocation:(CLLocationCoordinate2D const &)location
{
  self = [super init];
  if (self)
  {
    _location = location;
    _isMyPosition = YES;
  }
  return self;
}

- (instancetype)initForPlacePageObject:(id<MWMPlacePageObject>)object
{
  NSAssert(false, @"deprecated");

  return nil;
}

- (instancetype)initForPlacePage:(PlacePageData *)data {
  self = [super init];
  if (self)
  {
    NSAssert(data, @"Entity can't be nil!");
    _isMyPosition = data.isMyPosition;
    _data = data;
  }
  return self;
}

- (NSString *)url:(BOOL)isShort
{
  auto & f = GetFramework();

  auto const title = ^NSString *(PlacePageData *data)
  {
    if (!data || data.isMyPosition)
      return L(@"share_my_position");
    else if (data.previewData.title.length > 0)
      return data.previewData.title;
    else if (data.previewData.subtitle.length)
      return data.previewData.subtitle;
    else if (data.previewData.address.length)
      return data.previewData.address;
    else
      return @"";
  };

  ms::LatLon const ll = self.data ? ms::LatLon(self.data.locationCoordinate.latitude,
                                               self.data.locationCoordinate.longitude)
                                    : ms::LatLon(self.location.latitude, self.location.longitude);
  std::string const & s = f.CodeGe0url(ll.m_lat, ll.m_lon, f.GetDrawScale(), title(self.data).UTF8String);

  NSString * url = @(s.c_str());
  if (!isShort)
    return url;
  NSUInteger const kGe0UrlLength = 16;
  return [url substringWithRange:NSMakeRange(0, kGe0UrlLength)];
}

#pragma mark - UIActivityItemSource

- (id)activityViewControllerPlaceholderItem:(UIActivityViewController *)activityViewController
{
  return [self url:YES];
}

- (id)activityViewController:(UIActivityViewController *)activityViewController
         itemForActivityType:(NSString *)activityType
{
  NSString * type = activityType;
  if ([UIActivityTypePostToTwitter isEqualToString:type])
    return self.itemForTwitter;
  return [self itemDefaultWithActivityType:type];
}

- (NSString *)activityViewController:(UIActivityViewController *)activityViewController
              subjectForActivityType:(NSString *)activityType
{
  return [self subjectDefault];
}

#pragma mark - Message

- (NSString *)itemForTwitter
{
  NSString * shortUrl = [self url:YES];
  return [NSString stringWithFormat:@"%@\n%@", httpGe0Url(shortUrl),
                                    self.isMyPosition ? L(@"share_my_position")
                                                      : self.data.previewData.title];
}

- (NSString *)itemDefaultWithActivityType:(NSString *)activityType
{
  NSString * ge0Url = [self url:NO];
  NSString * url = httpGe0Url(ge0Url);
  if (self.isMyPosition)
  {
    BOOL const hasSubject = [activityType isEqualToString:UIActivityTypeMail];
    if (hasSubject)
      return url;
    return [NSString
        stringWithFormat:@"%@ %@", L(@"share_my_position"), url];
  }

  NSMutableString * result = [@"" mutableCopy];
  PlacePagePreviewData * pd = self.data.previewData;
  for (NSString * str in @[pd.title, pd.subtitle, pd.address, self.data.infoData.phone, url])
  {
    if (str.length)
    {
      if (result.length)
        [result appendString:@"\n"];
      [result appendString:str];
    }
  }

  return result;
}

#pragma mark - Subject

- (NSString *)subjectDefault
{
  return self.isMyPosition ? L(@"share_my_position")
                           : L(@"share_coords_subject_default");
}

@end
