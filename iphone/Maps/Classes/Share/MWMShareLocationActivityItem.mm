#import "Macros.h"
#import "MWMShareLocationActivityItem.h"
#import "Statistics.h"

#import "3party/Alohalytics/src/alohalytics_objc.h"

#include "Framework.h"

NSString * httpGe0Url(NSString * shortUrl)
{
  return [shortUrl stringByReplacingCharactersInRange:NSMakeRange(0, 6) withString:@"http://ge0.me/"];
}

@interface MWMShareLocationActivityItem ()

@property (copy, nonatomic) NSString * title;
@property (nonatomic) CLLocationCoordinate2D location;
@property (nonatomic) BOOL myPosition;

@end

@implementation MWMShareLocationActivityItem

- (instancetype)initWithTitle:(NSString *)title location:(CLLocationCoordinate2D)location myPosition:(BOOL)myPosition
{
  self = [super init];
  if (self)
  {
    self.title = title ? title : @"";
    self.location = location;
    self.myPosition = myPosition;
  }
  return self;
}

- (NSString *)url:(BOOL)isShort
{
  auto & f = GetFramework();
  string const s = f.CodeGe0url(self.location.latitude, self.location.longitude, f.GetDrawScale(),
                                self.title.UTF8String);
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
  [Statistics logEvent:kStatEventName(kStatShare, kStatLocation) withParameters:@{kStatAction : activityType}];
  [Alohalytics logEvent:event withValue:activityType];
  if ([UIActivityTypeMessage isEqualToString:activityType])
    return [self itemForMessageApp];
  if ([UIActivityTypeMail isEqualToString:activityType])
    return [self itemForMailApp];
  return [self itemDefault];
}

- (NSString *)activityViewController:(UIActivityViewController *)activityViewController
              subjectForActivityType:(NSString *)activityType
{
  return [self subjectDefault];
}

#pragma mark - Message

- (NSString *)itemForMessageApp
{
  NSString * shortUrl = [self url:YES];
  return [NSString stringWithFormat:self.myPosition ? L(@"my_position_share_sms") : L(@"bookmark_share_sms"),
          shortUrl, httpGe0Url(shortUrl)];
}

- (NSString *)itemForMailApp
{
  NSString * url = [self url:NO];
  if (!self.myPosition)
    return [NSString stringWithFormat:L(@"bookmark_share_email"), self.title, url, httpGe0Url(url)];

  search::AddressInfo const info = GetFramework().GetAddressInfoAtPoint(
                                              MercatorBounds::FromLatLon(self.location.latitude, self.location.longitude));

  NSString * nameAndAddress = @(info.FormatNameAndAddress().c_str());
  return [NSString stringWithFormat:L(@"my_position_share_email"), nameAndAddress, url, httpGe0Url(url)];
}

- (NSString *)itemDefault
{
  return httpGe0Url([self url:NO]);
}

#pragma mark - Subject

- (NSString *)subjectDefault
{
  return self.myPosition ? L(@"my_position_share_email_subject") : L(@"bookmark_share_email_subject");
}

@end