#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "MapViewController.h"

#include "platform/measurement_utils.hpp"
#include "indexer/osm_editor.hpp"

using feature::Metadata;

extern NSString * const kUserDefaultsLatLonAsDMSKey = @"UserDefaultsLatLonAsDMS";

static array<MWMPlacePageMetadataField, 10> const kPatternFieldsArray{
    {MWMPlacePageMetadataFieldPostcode,
     MWMPlacePageMetadataFieldPhoneNumber,
     MWMPlacePageMetadataFieldWebsite,
     MWMPlacePageMetadataFieldURL,
     MWMPlacePageMetadataFieldEmail,
     MWMPlacePageMetadataFieldOpenHours,
     MWMPlacePageMetadataFieldWiFi,
     MWMPlacePageMetadataFieldCoordinate,
     MWMPlacePageMetadataFieldBookmark,
     MWMPlacePageMetadataFieldEditButton}};

static map<Metadata::EType, MWMPlacePageMetadataField> const kMetaFieldsMap{
    {Metadata::FMD_URL, MWMPlacePageMetadataFieldURL},
    {Metadata::FMD_WEBSITE, MWMPlacePageMetadataFieldWebsite},
    {Metadata::FMD_PHONE_NUMBER, MWMPlacePageMetadataFieldPhoneNumber},
    {Metadata::FMD_OPEN_HOURS, MWMPlacePageMetadataFieldOpenHours},
    {Metadata::FMD_EMAIL, MWMPlacePageMetadataFieldEmail},
    {Metadata::FMD_POSTCODE, MWMPlacePageMetadataFieldPostcode},
    {Metadata::FMD_INTERNET, MWMPlacePageMetadataFieldWiFi}};

@implementation MWMPlacePageEntity
{
  vector<MWMPlacePageMetadataField> m_fields;
  set<MWMPlacePageMetadataField> m_editableFields;
  map<MWMPlacePageMetadataField, string> m_values;
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
    case Type::POI:
      [self configureEntityWithFeature:mark->GetFeature()];
      break;
    case Type::BOOKMARK:
      [self configureForBookmark:mark];
      break;
  }
  [self setEditableFields];
  [self sortMetaFields];
}

- (void)sortMetaFields
{
  auto begin = kPatternFieldsArray.begin();
  auto end = kPatternFieldsArray.end();
  sort(m_fields.begin(), m_fields.end(), [&](MWMPlacePageMetadataField a, MWMPlacePageMetadataField b)
  {
    return find(begin, end, a) < find(begin, end, b);
  });
}

- (void)addMetaField:(MWMPlacePageMetadataField)field
{
  if (find(m_fields.begin(), m_fields.end(), field) == m_fields.end())
    m_fields.emplace_back(field);
}

- (void)removeMetaField:(MWMPlacePageMetadataField)field
{
  auto it = find(m_fields.begin(), m_fields.end(), field);
  if (it != m_fields.end())
    m_fields.erase(it);
}

- (void)addMetaField:(MWMPlacePageMetadataField)field value:(string const &)value
{
  [self addMetaField:field];
  m_values.emplace(field, value);
}

- (void)configureForBookmark:(UserMark const *)bookmark
{
  Framework & f = GetFramework();
  self.bac = f.FindBookmark(bookmark);
  self.type = MWMPlacePageEntityTypeBookmark;
  BookmarkCategory * category = f.GetBmCategory(self.bac.first);
  BookmarkData const & data = static_cast<Bookmark const *>(bookmark)->GetData();

  self.bookmarkTitle = @(data.GetName().c_str());
  self.bookmarkCategory = @(category->GetName().c_str());
  string const description = data.GetDescription();
  self.bookmarkDescription = @(description.c_str());
  _isHTMLDescription = strings::IsHTML(description);
  self.bookmarkColor = @(data.GetType().c_str());

  [self configureEntityWithFeature:bookmark->GetFeature()];
  [self addBookmarkField];
}

- (void)configureForMyPosition:(MyPositionMarkPoint const *)myPositionMark
{
  self.title = L(@"my_position");
  self.type = MWMPlacePageEntityTypeMyPosition;
  [self addMetaField:MWMPlacePageMetadataFieldCoordinate];
}

- (void)configureForApi:(ApiMarkPoint const *)apiMark
{
  self.type = MWMPlacePageEntityTypeAPI;
  self.title = @(apiMark->GetName().c_str());
  self.category = @(GetFramework().GetApiDataHolder().GetAppTitle().c_str());
  [self addMetaField:MWMPlacePageMetadataFieldCoordinate];
}

// feature can be nullptr if user selected any empty area.
- (void)configureEntityWithFeature:(FeatureType const *)feature
{
  if (feature)
  {
    search::AddressInfo const info = GetFramework().GetPOIAddressInfo(*feature);
    feature::Metadata const & metadata = feature->GetMetadata();
    NSString * const name = @(info.GetPinName().c_str());
    self.title = name.length > 0 ? name : L(@"dropped_pin");
    self.category = @(info.GetPinType().c_str());

    vector<Metadata::EType> const presentTypes = metadata.GetPresentTypes();

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
          [self addMetaField:kMetaFieldsMap.at(type) value:metadata.Get(type)];
          break;
        case Metadata::FMD_INTERNET:
          [self addMetaField:kMetaFieldsMap.at(type) value:L(@"WiFi_available").UTF8String];
          break;
        default:
          break;
      }
    }
  }
  [self addMetaField:MWMPlacePageMetadataFieldCoordinate];
}

- (void)addEditField
{
  [self addMetaField:MWMPlacePageMetadataFieldEditButton];
}

- (void)addBookmarkField
{
  [self addMetaField:MWMPlacePageMetadataFieldBookmark];
  [self sortMetaFields];
}

- (void)removeBookmarkField
{
  [self removeMetaField:MWMPlacePageMetadataFieldBookmark];
}

- (void)toggleCoordinateSystem
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setBool:![ud boolForKey:kUserDefaultsLatLonAsDMSKey] forKey:kUserDefaultsLatLonAsDMSKey];
  [ud synchronize];
}

#pragma mark - Editing

- (void)setEditableFields
{
  // TODO: Replace with real array
  vector<Metadata::EType> const editableTypes {Metadata::FMD_OPEN_HOURS};
  //osm::Editor::Instance().EditableMetadataForType(<#const FeatureType &feature#>);
  if (!editableTypes.empty())
    [self addEditField];
  for (auto const & type : editableTypes)
    m_editableFields.insert(kMetaFieldsMap.at(type));
}

- (BOOL)isFieldEditable:(MWMPlacePageMetadataField)field
{
  return m_editableFields.count(field) == 1;
}

#pragma mark - Getters

- (NSUInteger)getFieldsCount
{
  return m_fields.size();
}

- (MWMPlacePageMetadataField)getFieldType:(NSUInteger)index
{
  NSAssert(index < [self getFieldsCount], @"Invalid meta index");
  return m_fields[index];
}

- (NSString *)getFieldValue:(MWMPlacePageMetadataField)field
{
  if (field == MWMPlacePageMetadataFieldCoordinate)
    return [self coordinate];
  auto it = m_values.find(field);
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
