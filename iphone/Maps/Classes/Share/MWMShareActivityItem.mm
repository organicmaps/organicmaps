#import "MWMShareActivityItem.h"
#import "MWMPlacePageEntity.h"
#import "Macros.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

NSString * httpGe0Url(NSString * shortUrl)
{
  return
      [shortUrl stringByReplacingCharactersInRange:NSMakeRange(0, 6) withString:@"http://ge0.me/"];
}

@interface MWMShareActivityItem ()

@property(nonatomic) MWMPlacePageEntity * entity;
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

- (instancetype)initForPlacePageObjectWithEntity:(MWMPlacePageEntity *)entity
{
  self = [super init];
  if (self)
  {
    NSAssert(entity, @"Entity can't be nil!");
    BOOL const isMyPosition = entity.isMyPosition;
    _isMyPosition = isMyPosition;
    if (!isMyPosition)
      _entity = entity;
  }
  return self;
}

- (NSString *)url:(BOOL)isShort
{
  auto & f = GetFramework();

  auto const title = ^NSString *(MWMPlacePageEntity * entity)
  {
    if (!entity || entity.isMyPosition)
      return L(@"my_position");
    else if (entity.title.length)
      return entity.title;
    else if (entity.subtitle.length)
      return entity.subtitle;
    else if (entity.address.length)
      return entity.address;
    else
      return @"";
  };

  ms::LatLon const ll = self.entity ? self.entity.latlon : ms::LatLon(self.location.latitude, self.location.longitude);
  string const s = f.CodeGe0url(ll.lat, ll.lon, f.GetDrawScale(), title(self.entity).UTF8String);
  
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
  [Statistics logEvent:kStatEventName(kStatShare, kStatLocation)
        withParameters:@{kStatAction : activityType}];
  [Alohalytics logEvent:event withValue:activityType];
  if ([UIActivityTypePostToTwitter isEqualToString:activityType])
    return self.itemForTwitter;
  return [self itemDefaultWithActivityType:activityType];
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
                                                      : self.entity.title];
}

- (NSString *)itemDefaultWithActivityType:(NSString *)activityType
{
  NSString * url = httpGe0Url([self url:NO]);
  if (self.isMyPosition)
  {
    BOOL const hasSubject = [activityType isEqualToString:UIActivityTypeMail];
    if (hasSubject)
      return url;
    return [NSString stringWithFormat:@"%@ %@", L(@"my_position_share_email_subject"), url];
  }

  NSMutableString * result = [L(@"sharing_call_action_look") mutableCopy];
  vector<NSString *> strings{self.entity.title, self.entity.subtitle, self.entity.address,
                             [self.entity getCellValue:MWMPlacePageCellTypePhoneNumber], url};

  if (self.entity.isBooking)
  {
    strings.push_back(L(@"sharing_booking"));
    strings.push_back(self.entity.bookingDescriptionUrl.absoluteString);
  }

  for (auto const str : strings)
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