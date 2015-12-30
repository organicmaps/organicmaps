#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "MapViewController.h"

#include "platform/measurement_utils.hpp"
#include "indexer/osm_editor.hpp"

using feature::Metadata;

extern NSString * const kUserDefaultsLatLonAsDMSKey = @"UserDefaultsLatLonAsDMS";

static array<MWMPlacePageMetadataType, 8> const kPatternTypesArray{
    {MWMPlacePageMetadataTypePostcode, MWMPlacePageMetadataTypePhoneNumber,
     MWMPlacePageMetadataTypeWebsite, MWMPlacePageMetadataTypeURL, MWMPlacePageMetadataTypeEmail,
     MWMPlacePageMetadataTypeOpenHours, MWMPlacePageMetadataTypeWiFi,
     MWMPlacePageMetadataTypeCoordinate}};

static map<Metadata::EType, MWMPlacePageMetadataType> const kMetaTypesMap{
    {Metadata::FMD_URL, MWMPlacePageMetadataTypeURL},
    {Metadata::FMD_WEBSITE, MWMPlacePageMetadataTypeWebsite},
    {Metadata::FMD_PHONE_NUMBER, MWMPlacePageMetadataTypePhoneNumber},
    {Metadata::FMD_OPEN_HOURS, MWMPlacePageMetadataTypeOpenHours},
    {Metadata::FMD_EMAIL, MWMPlacePageMetadataTypeEmail},
    {Metadata::FMD_POSTCODE, MWMPlacePageMetadataTypePostcode},
    {Metadata::FMD_INTERNET, MWMPlacePageMetadataTypeWiFi}};

@implementation MWMPlacePageEntity
{
  vector<MWMPlacePageMetadataType> m_types;
  map<MWMPlacePageMetadataType, string> m_values;
}

- (instancetype)initWithUserMark:(UserMark const *)userMark
{
  NSAssert(userMark, @"userMark can not be nil.");
  self = [super init];
  if (self)
    [self configureWithUserMark:userMark];
  return self;
}

- (void)configureWithUserMark:(UserMark const *)mark
{
  _latlon = mark->GetLatLon();
  using Type = UserMark::Type;
  switch (mark->GetMarkType())
  {
    case Type::API:
      [self configureForApi:static_cast<ApiMarkPoint const *>(mark)];
      break;
    case Type::DEBUG_MARK:
      break;
    case Type::MY_POSITION:
      [self configureForMyPosition:static_cast<MyPositionMarkPoint const *>(mark)];
      break;
    case Type::SEARCH:
      [self configureForSearch:static_cast<SearchMarkPoint const *>(mark)];
      break;
    case Type::POI:
      [self configureForPOI:static_cast<PoiMarkPoint const *>(mark)];
      break;
    case Type::BOOKMARK:
      [self configureForBookmark:mark];
      break;
  }
  [self addEditableTypes];
  [self sortMeta];
}

- (void)sortMeta
{
  auto begin = kPatternTypesArray.begin();
  auto end = kPatternTypesArray.end();
  sort(m_types.begin(), m_types.end(), [&](MWMPlacePageMetadataType a, MWMPlacePageMetadataType b)
  {
    return find(begin, end, a) < find(begin, end, b);
  });
}

- (void)addMetaType:(MWMPlacePageMetadataType)type
{
  if (find(m_types.begin(), m_types.end(), type) == m_types.end())
    m_types.emplace_back(type);
}

- (void)removeMetaType:(MWMPlacePageMetadataType)type
{
  auto it = find(m_types.begin(), m_types.end(), type);
  if (it != m_types.end())
    m_types.erase(it);
}

- (void)addMetaType:(MWMPlacePageMetadataType)type value:(string const &)value
{
  [self addMetaType:type];
  m_values.emplace(type, value);
}

- (void)configureForBookmark:(UserMark const *)bookmark
{
  Framework & f = GetFramework();
  self.bac = f.FindBookmark(bookmark);
  self.type = MWMPlacePageEntityTypeBookmark;
  BookmarkCategory * category = f.GetBmCategory(self.bac.first);
  BookmarkData const & data = static_cast<Bookmark const *>(bookmark)->GetData();
  m2::PointD const & point = bookmark->GetPivot();
  Metadata metadata;
  search::AddressInfo info;
  f.FindClosestPOIMetadata(point, metadata);
  f.GetAddressInfoForGlobalPoint(point, info);

  self.bookmarkTitle = @(data.GetName().c_str());
  self.bookmarkCategory = @(category->GetName().c_str());
  string const description = data.GetDescription();
  self.bookmarkDescription = @(description.c_str());
  _isHTMLDescription = strings::IsHTML(description);
  self.bookmarkColor = @(data.GetType().c_str());

  [self configureEntityWithMetadata:metadata addressInfo:info];
  [self insertBookmarkInTypes];
}

- (void)configureForSearch:(SearchMarkPoint const *)searchMark
{
//Workaround for framework bug.
//TODO: Make correct way to get search metadata.
  Metadata metadata;
  GetFramework().FindClosestPOIMetadata(searchMark->GetPivot(), metadata);
  [self configureEntityWithMetadata:metadata addressInfo:searchMark->GetInfo()];
}

- (void)configureForPOI:(PoiMarkPoint const *)poiMark
{
  [self configureEntityWithMetadata:poiMark->GetMetadata() addressInfo:poiMark->GetInfo()];
}

- (void)configureForMyPosition:(MyPositionMarkPoint const *)myPositionMark
{
  self.title = L(@"my_position");
  self.type = MWMPlacePageEntityTypeMyPosition;
  [self addMetaType:MWMPlacePageMetadataTypeCoordinate];
}

- (void)configureForApi:(ApiMarkPoint const *)apiMark
{
  self.type = MWMPlacePageEntityTypeAPI;
  self.title = @(apiMark->GetName().c_str());
  self.category = @(GetFramework().GetApiDataHolder().GetAppTitle().c_str());
  [self addMetaType:MWMPlacePageMetadataTypeCoordinate];
}

- (void)configureEntityWithMetadata:(Metadata const &)metadata addressInfo:(search::AddressInfo const &)info
{
  NSString * const name = @(info.GetPinName().c_str());
  self.title = name.length > 0 ? name : L(@"dropped_pin");
  self.category = @(info.FormatAddress().c_str());

  auto const presentTypes = metadata.GetPresentTypes();

  for (auto const & type : presentTypes)
  {
    switch (type)
    {
      case Metadata::FMD_CUISINE:
      {
        NSString * result = @(metadata.Get(type).c_str());
        NSString * cuisine = [NSString stringWithFormat:@"cuisine_%@", result];
        NSString * localizedResult = L(cuisine);
        NSString * currentCategory = self.category;
        if (![localizedResult isEqualToString:currentCategory])
        {
          if ([localizedResult isEqualToString:cuisine])
          {
            if (![result isEqualToString:currentCategory])
              self.category = [NSString stringWithFormat:@"%@, %@", self.category, result];
          }
          else
          {
            self.category = [NSString stringWithFormat:@"%@, %@", self.category, localizedResult];
          }
        }
        break;
      }
      case Metadata::FMD_ELE:
      {
        self.typeDescriptionValue = atoi(metadata.Get(type).c_str());
        if (self.type != MWMPlacePageEntityTypeBookmark)
          self.type = MWMPlacePageEntityTypeEle;
        break;
      }
      case Metadata::FMD_OPERATOR:
      {
        NSString const * bank = @(metadata.Get(type).c_str());
        if (self.category.length)
          self.category = [NSString stringWithFormat:@"%@, %@", self.category, bank];
        else
          self.category = [NSString stringWithFormat:@"%@", bank];
        break;
      }
      case Metadata::FMD_STARS:
      {
        self.typeDescriptionValue = atoi(metadata.Get(type).c_str());
        if (self.type != MWMPlacePageEntityTypeBookmark)
          self.type = MWMPlacePageEntityTypeHotel;
        break;
      }
      case Metadata::FMD_URL:
      case Metadata::FMD_WEBSITE:
      case Metadata::FMD_PHONE_NUMBER:
      case Metadata::FMD_OPEN_HOURS:
      case Metadata::FMD_EMAIL:
      case Metadata::FMD_POSTCODE:
        [self addMetaType:kMetaTypesMap.at(type) value:metadata.Get(type)];
        break;
      case Metadata::FMD_INTERNET:
        [self addMetaType:kMetaTypesMap.at(type) value:L(@"WiFi_available").UTF8String];
        break;
      default:
        break;
    }
  }

  [self addMetaType:MWMPlacePageMetadataTypeCoordinate];
}

- (void)enableEditing
{
  [self addMetaType:MWMPlacePageMetadataTypeEditButton];
}

- (void)insertBookmarkInTypes
{
  [self addMetaType:MWMPlacePageMetadataTypeBookmark];
}

- (void)removeBookmarkFromTypes
{
  [self removeMetaType:MWMPlacePageMetadataTypeBookmark];
}

- (void)toggleCoordinateSystem
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setBool:![ud boolForKey:kUserDefaultsLatLonAsDMSKey] forKey:kUserDefaultsLatLonAsDMSKey];
  [ud synchronize];
}

#pragma mark - Editing

- (void)addEditableTypes
{
  // TODO: Add real logic
}

#pragma mark - Getters

- (NSUInteger)getFeatureTypesCount
{
  return m_types.size();
}

- (MWMPlacePageMetadataType)getFeatureType:(NSUInteger)index
{
  NSAssert(index < [self getFeatureTypesCount], @"Invalid meta index");
  return m_types[index];
}

- (NSString *)getFeatureValue:(MWMPlacePageMetadataType)type
{
  if (type == MWMPlacePageMetadataTypeCoordinate)
    return [self coordinate];
  auto it = m_values.find(type);
  return it != m_values.end() ? @(it->second.c_str()) : nil;
}

- (NSString *)coordinate
{
  BOOL const useDMSFormat =
      [[NSUserDefaults standardUserDefaults] boolForKey:kUserDefaultsLatLonAsDMSKey];
  ms::LatLon const latlon = self.latlon;
  return @((useDMSFormat ? MeasurementUtils::FormatLatLon(latlon.lat, latlon.lon)
                         : MeasurementUtils::FormatLatLonAsDMS(latlon.lat, latlon.lon, 2))
               .c_str());
}

#pragma mark - Bookmark editing

- (NSString *)bookmarkCategory
{
  if (!_bookmarkCategory)
  {
    Framework & f = GetFramework();
    BookmarkCategory * category = f.GetBmCategory(f.LastEditedBMCategory());
    _bookmarkCategory = @(category->GetName().c_str());
  }
  return _bookmarkCategory;
}

- (NSString *)bookmarkDescription
{
  if (!_bookmarkDescription)
    _bookmarkDescription = @"";
  return _bookmarkDescription;
}

- (NSString *)bookmarkColor
{
  if (!_bookmarkColor)
  {
    Framework & f = GetFramework();
    string type = f.LastEditedBMType();
    _bookmarkColor = @(type.c_str());
  }
  return _bookmarkColor;
}

- (NSString *)bookmarkTitle
{
  if (!_bookmarkTitle)
    _bookmarkTitle = self.title;
  return _bookmarkTitle;
}

- (void)synchronize
{
  Framework & f = GetFramework();
  BookmarkCategory * category = f.GetBmCategory(self.bac.first);
  if (!category)
    return;

  {
    BookmarkCategory::Guard guard(*category);
    Bookmark * bookmark = static_cast<Bookmark *>(guard.m_controller.GetUserMarkForEdit(self.bac.second));
    if (!bookmark)
      return;
  
    if (self.bookmarkColor)
      bookmark->SetType(self.bookmarkColor.UTF8String);

    if (self.bookmarkDescription)
    {
      string const description(self.bookmarkDescription.UTF8String);
      _isHTMLDescription = strings::IsHTML(description);
      bookmark->SetDescription(description);
    }

    if (self.bookmarkTitle)
      bookmark->SetName(self.bookmarkTitle.UTF8String);
  }
  
  category->SaveToKMLFile();
}

@end
