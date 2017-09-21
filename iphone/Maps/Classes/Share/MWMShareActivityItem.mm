#import "MWMShareActivityItem.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

NSString * httpGe0Url(NSString * shortUrl)
{
  return
      [shortUrl stringByReplacingCharactersInRange:NSMakeRange(0, 6) withString:@"http://ge0.me/"];
}

@interface MWMShareActivityItem ()

@property(nonatomic) id<MWMPlacePageObject> object;
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
  self = [super init];
  if (self)
  {
    NSAssert(object, @"Entity can't be nil!");
    BOOL const isMyPosition = object.isMyPosition;
    _isMyPosition = isMyPosition;
    _object = object;
  }
  return self;
}

- (NSString *)url:(BOOL)isShort
{
  auto & f = GetFramework();

  auto const title = ^NSString *(id<MWMPlacePageObject> obj)
  {
    if (!obj || obj.isMyPosition)
      return L(@"my_position");
    else if (obj.title.length)
      return obj.title;
    else if (obj.subtitle.length)
      return obj.subtitle;
    else if (obj.address.length)
      return obj.address;
    else
      return @"";
  };

  ms::LatLon const ll = self.object ? self.object.latLon
                                    : ms::LatLon(self.location.latitude, self.location.longitude);
  string const & s = f.CodeGe0url(ll.lat, ll.lon, f.GetDrawScale(), title(self.object).UTF8String);

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
  NSString * event = @"MWMShareLocationActivityItem:activityViewController:itemForActivityType:";
  NSString * type = activityType;
  [Statistics logEvent:kStatEventName(kStatShare, kStatLocation)
        withParameters:@{kStatAction : type}];
  [Alohalytics logEvent:event withValue:type];
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
                                    self.isMyPosition ? L(@"my_position_share_email_subject")
                                                      : self.object.title];
}

- (NSString *)itemDefaultWithActivityType:(NSString *)activityType
{
  NSString * ge0Url = [self url:NO];
  NSString * url = httpGe0Url(ge0Url);
  if (self.isMyPosition)
  {
    BOOL const hasSubject = [activityType isEqualToString:UIActivityTypeMail];
    if (hasSubject)
      return [NSString stringWithFormat:@"%@ %@", url, ge0Url];
    return [NSString
        stringWithFormat:@"%@ %@\n%@", L(@"my_position_share_email_subject"), url, ge0Url];
  }

  NSMutableString * result = [L(@"sharing_call_action_look") mutableCopy];
  vector<NSString *> strings{self.object.title,
                             self.object.subtitle,
                             self.object.address,
                             self.object.phoneNumber,
                             url,
                             ge0Url};

  if (self.object.isBooking)
  {
    strings.push_back(L(@"sharing_booking"));
    strings.push_back(self.object.sponsoredDescriptionURL.absoluteString);
  }

  for (auto const & str : strings)
  {
    if (str.length)
      [result appendString:[NSString stringWithFormat:@"\n%@", str]];
  }

  return result;
}

#pragma mark - Subject

- (NSString *)subjectDefault
{
  return self.isMyPosition ? L(@"my_position_share_email_subject")
                           : L(@"bookmark_share_email_subject");
}

@end
