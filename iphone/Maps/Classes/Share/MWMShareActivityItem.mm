#import "MWMShareActivityItem.h"

#import <CoreApi/Framework.h>
#import <CoreApi/PlacePageData.h>
#import <CoreApi/PlacePagePreviewData.h>
#import <CoreApi/PlacePageInfoData.h>

@interface MWMShareActivityItem ()

@property(nonatomic) NSString * url;           // https://omaps.app/XXXYYY/Title
@property(nonatomic) NSString * coordinates;   // 12.12313, -43.431234
@property(nonatomic) NSString * message;       // [Title] \n[subtitle] \n[addr] \nurl
@property(nonatomic) NSString * emailSubject;  // Used only if location is shared via Email.

@end

// Returns https://omaps.app/XXXYYY/Title or https://omaps.app/XXXYYY if title is empty.
static NSString * OmapsUrl(CLLocationCoordinate2D loc, NSString * _Nonnull title)
{
  ms::LatLon const ll = ms::LatLon(loc.latitude, loc.longitude);
  auto & f = GetFramework();
  NSString * omUrl = @(f.CodeGe0url(ll.m_lat, ll.m_lon, f.GetDrawScale(), title.UTF8String).c_str());
  // Replace "om://" with "https://omaps.app/".
  return [omUrl stringByReplacingCharactersInRange:NSMakeRange(0, 5) withString:@"https://omaps.app/"];
}


@implementation MWMShareActivityItem

- (instancetype)initForMyPositionAtLocation:(CLLocationCoordinate2D)loc
{
  if (self = [super init])
  {
    _url = OmapsUrl(loc, L(@"share_my_position"));
    auto & f = GetFramework();
    auto const address = f.GetAddressAtPoint(mercator::FromLatLon(loc.latitude, loc.longitude), 10 /*meters*/);
    if (address.IsValid())
    {
      _emailSubject = @(address.FormatAddress().c_str());
      _message = [NSString stringWithFormat:@"%@ \n%@", _emailSubject, _url];
    }
    else
    {
      _message = _url;
      _emailSubject = L(@"share_my_position");
    }
  }
  return self;
}

- (instancetype)initForPlacePage:(PlacePageData * _Nonnull)data
{
  if (data.isMyPosition)
    return [self initForMyPositionAtLocation:data.locationCoordinate];

  if (self = [super init])
  {
    auto const urlTitle = ^NSString *(PlacePagePreviewData * pdata)
    {
      if (pdata.title.length && ![pdata.title isEqualToString:L(@"core_placepage_unknown_place")])
        return pdata.title;
      else if (pdata.subtitle.length)
        return pdata.subtitle;
      else if (pdata.address.length)
        return pdata.address;
      return @"";
    };
    _url = OmapsUrl(data.locationCoordinate, urlTitle(data.previewData));
    auto const text = ^NSString *(PlacePagePreviewData * pdata, NSString * delim)
    {
      NSMutableString * mstr = [NSMutableString stringWithCapacity:100];
      if (pdata.title.length && ![pdata.title isEqualToString:L(@"core_placepage_unknown_place")])
        [mstr appendFormat:@"%@%@", pdata.title, delim];
      if (pdata.subtitle.length)
        [mstr appendFormat:@"%@%@", pdata.subtitle, delim];
      if (pdata.address.length)
        [mstr appendFormat:@"%@%@", pdata.address, delim];
      return mstr;
    };
    _message = [NSString stringWithFormat:@"%@ %@", text(data.previewData, @" \n"), _url];
    _emailSubject = text(data.previewData, @" ");
    if (_emailSubject.length == 0)
      _emailSubject = L(@"share_coords_subject_default");
  }
  return self;
}

#pragma mark - UIActivityItemSource

- (id)activityViewControllerPlaceholderItem:(UIActivityViewController *)activityViewController
{
  return self.url;
}

- (id)activityViewController:(UIActivityViewController *)activityViewController
         itemForActivityType:(NSString *)activityType
{
  NSString * type = activityType;
  if ([UIActivityTypePostToTwitter isEqualToString:type] ||
      [UIActivityTypeCopyToPasteboard isEqualToString:type])
    return self.url;

  return self.message;
}

- (NSString *)activityViewController:(UIActivityViewController *)activityViewController
              subjectForActivityType:(NSString *)activityType
{
  return self.emailSubject;
}

@end
