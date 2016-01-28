#import "MWMPlacePageEntity.h"
#import "MWMPlacePageViewManager.h"
#import "MapViewController.h"

#include "platform/measurement_utils.hpp"
#include "indexer/osm_editor.hpp"

using feature::Metadata;

extern NSString * const kUserDefaultsLatLonAsDMSKey = @"UserDefaultsLatLonAsDMS";

namespace
{

NSString * const kOSMCuisineSeparator = @";";
NSString * const kMWMCuisineSeparator = @" • ";

array<MWMPlacePageCellType, 10> const gMetaFieldsMap{
    {MWMPlacePageCellTypePostcode, MWMPlacePageCellTypePhoneNumber, MWMPlacePageCellTypeWebsite,
     MWMPlacePageCellTypeURL, MWMPlacePageCellTypeEmail, MWMPlacePageCellTypeOpenHours,
     MWMPlacePageCellTypeWiFi, MWMPlacePageCellTypeCoordinate, MWMPlacePageCellTypeBookmark,
     MWMPlacePageCellTypeEditButton}};

NSUInteger kMetaFieldsMap[MWMPlacePageCellTypeCount] = {};

void putFields(NSUInteger eTypeValue, NSUInteger ppValue)
{
  kMetaFieldsMap[eTypeValue] = ppValue;
  kMetaFieldsMap[ppValue] = eTypeValue;
}

void initFieldsMap()
{
  putFields(Metadata::FMD_URL, MWMPlacePageCellTypeURL);
  putFields(Metadata::FMD_WEBSITE, MWMPlacePageCellTypeWebsite);
  putFields(Metadata::FMD_PHONE_NUMBER, MWMPlacePageCellTypePhoneNumber);
  putFields(Metadata::FMD_OPEN_HOURS, MWMPlacePageCellTypeOpenHours);
  putFields(Metadata::FMD_EMAIL, MWMPlacePageCellTypeEmail);
  putFields(Metadata::FMD_POSTCODE, MWMPlacePageCellTypePostcode);
  putFields(Metadata::FMD_INTERNET, MWMPlacePageCellTypeWiFi);
  putFields(Metadata::FMD_CUISINE, MWMPlacePageCellTypeCuisine);

  ASSERT_EQUAL(kMetaFieldsMap[Metadata::FMD_URL], MWMPlacePageCellTypeURL, ());
  ASSERT_EQUAL(kMetaFieldsMap[MWMPlacePageCellTypeURL], Metadata::FMD_URL, ());
  ASSERT_EQUAL(kMetaFieldsMap[Metadata::FMD_WEBSITE], MWMPlacePageCellTypeWebsite, ());
  ASSERT_EQUAL(kMetaFieldsMap[MWMPlacePageCellTypeWebsite], Metadata::FMD_WEBSITE, ());
  ASSERT_EQUAL(kMetaFieldsMap[Metadata::FMD_POSTCODE], MWMPlacePageCellTypePostcode, ());
  ASSERT_EQUAL(kMetaFieldsMap[MWMPlacePageCellTypePostcode], Metadata::FMD_POSTCODE, ());
  ASSERT_EQUAL(kMetaFieldsMap[MWMPlacePageCellTypeSpacer], 0, ());
  ASSERT_EQUAL(kMetaFieldsMap[Metadata::FMD_MAXSPEED], 0, ());
}

NSString * mwmToOSMCuisineString(NSString * mwmCuisine)
{
  return [mwmCuisine stringByReplacingOccurrencesOfString:kMWMCuisineSeparator withString:kOSMCuisineSeparator];
}
} // namespace

@interface MWMPlacePageEntity ()

@property (weak, nonatomic) id<MWMPlacePageEntityProtocol> delegate;

@end

@implementation MWMPlacePageEntity
{
  vector<MWMPlacePageCellType> m_fields;
  set<MWMPlacePageCellType> m_editableFields;
  MWMPlacePageCellTypeValueMap m_values;
}

+ (NSString *)makeMWMCuisineString:(NSSet<NSString *> *)cuisines
{
  NSString * prefix = @"cuisine_";
  NSMutableArray<NSString *> * localizedCuisines = [NSMutableArray arrayWithCapacity:cuisines.count];
  for (NSString * cus in cuisines)
  {
    NSString * cuisine = [prefix stringByAppendingString:cus];
    NSString * localizedCuisine = L(cuisine);
    BOOL const noLocalization = [localizedCuisine isEqualToString:cuisine];
    [localizedCuisines addObject:noLocalization ? cus : localizedCuisine];
  }
  [localizedCuisines sortUsingComparator:^NSComparisonResult(NSString * s1, NSString * s2)
  {
    return [s1 compare:s2 options:NSCaseInsensitiveSearch range:{0, s1.length} locale:[NSLocale currentLocale]];
  }];
  return [localizedCuisines componentsJoinedByString:kMWMCuisineSeparator];
}

- (instancetype)initWithDelegate:(id<MWMPlacePageEntityProtocol>)delegate
{
  NSAssert(delegate, @"delegate can not be nil.");
  self = [super init];
  if (self)
  {
    _delegate = delegate;
    initFieldsMap();
    [self config];
  }
  return self;
}

- (void)config
{
  UserMark const * mark = self.delegate.userMark;
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
      [self configureWithFeature:mark->GetFeature() andCustomName:nil];
      break;
    case Type::POI:
      [self configureWithFeature:mark->GetFeature()
                   andCustomName:@(static_cast<PoiMarkPoint const *>(mark)->GetCustomName().c_str())];
      break;
    case Type::BOOKMARK:
      [self configureForBookmark:mark];
      break;
  }
  [self setEditableTypes];
  [self sortMetaFields];
}

- (void)sortMetaFields
{
  auto const begin = gMetaFieldsMap.begin();
  auto const end = gMetaFieldsMap.end();
  sort(m_fields.begin(), m_fields.end(), [&](MWMPlacePageCellType a, MWMPlacePageCellType b)
  {
    return find(begin, end, a) < find(begin, end, b);
  });
}

- (void)addMetaField:(NSUInteger)value
{
  NSAssert(value >= Metadata::FMD_COUNT, @"Incorrect enum value");
  MWMPlacePageCellType const field = static_cast<MWMPlacePageCellType>(value);
  if (find(gMetaFieldsMap.begin(), gMetaFieldsMap.end(), field) == gMetaFieldsMap.end())
    return;
  if (find(m_fields.begin(), m_fields.end(), field) == m_fields.end())
    m_fields.emplace_back(field);
}

- (void)removeMetaField:(NSUInteger)value
{
  NSAssert(value >= Metadata::FMD_COUNT, @"Incorrect enum value");
  auto const it = find(m_fields.begin(), m_fields.end(), value);
  if (it != m_fields.end())
    m_fields.erase(it);
}

- (void)setMetaField:(NSUInteger)key value:(string const &)value
{
  NSAssert(key >= Metadata::FMD_COUNT, @"Incorrect enum value");
  MWMPlacePageCellType const cellType = static_cast<MWMPlacePageCellType>(key);
  if (value.empty())
  {
    [self removeMetaField:key];
    m_values.erase(cellType);
  }
  else
  {
    [self addMetaField:key];
    m_values[cellType] = value;
  }
}

- (void)configureForBookmark:(UserMark const *)bookmark
{
  // TODO: There is need to get address info which store feature address.
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

  [self configureWithFeature:bookmark->GetFeature() andCustomName:nil];
  [self addBookmarkField];
}

- (void)configureForMyPosition:(MyPositionMarkPoint const *)myPositionMark
{
  // TODO: There is need to get address info which store feature address.
  self.title = L(@"my_position");
  self.type = MWMPlacePageEntityTypeMyPosition;
  [self addMetaField:MWMPlacePageCellTypeCoordinate];
}

- (void)configureForApi:(ApiMarkPoint const *)apiMark
{
  // TODO: There is need to get address info which store feature address.
  self.type = MWMPlacePageEntityTypeAPI;
  self.title = @(apiMark->GetName().c_str());
  self.category = @(GetFramework().GetApiDataHolder().GetAppTitle().c_str());
  [self addMetaField:MWMPlacePageCellTypeCoordinate];
}

- (void)configureWithFeature:(FeatureType const *)feature andCustomName:(NSString *)customName
{
  NSString * emptyName = L(@"dropped_pin");
  // Custom name is used in shared links and should override default feature's name in PP.
  self.title = customName.length > 0 ? customName : emptyName;
  // feature can be nullptr if user selected any empty area.
  if (feature)
  {
    search::AddressInfo const info = GetFramework().GetFeatureAddressInfo(*feature);
    feature::Metadata const & metadata = feature->GetMetadata();
    NSString * const name = @(info.GetPinName().c_str());
    if (0 == self.title.length)
      self.title = name.length > 0 ? name : emptyName;
    self.category = @(info.GetPinType().c_str());
    self.address = @(info.FormatAddress().c_str());

    if (!info.m_house.empty())
      [self setMetaField:MWMPlacePageCellTypeBuilding value:info.m_house];

    for (auto const type : metadata.GetPresentTypes())
    {
      switch (type)
      {
        case Metadata::FMD_CUISINE:
        {
          [self deserializeCuisine:@(metadata.Get(type).c_str())];
          NSString * cuisine = [self getCellValue:MWMPlacePageCellTypeCuisine];
          if (self.category.length == 0)
            self.category = cuisine;
          else if (![self.category isEqualToString:cuisine])
            self.category = [NSString stringWithFormat:@"%@%@%@", self.category, kMWMCuisineSeparator, cuisine];
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
          NSString * bank = @(metadata.Get(type).c_str());
          if (self.category.length)
            self.category = [NSString stringWithFormat:@"%@%@%@", self.category, kMWMCuisineSeparator, bank];
          else
            self.category = bank;
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
          [self setMetaField:kMetaFieldsMap[type] value:metadata.Get(type)];
          break;
        case Metadata::FMD_INTERNET:
          [self setMetaField:kMetaFieldsMap[type] value:L(@"WiFi_available").UTF8String];
          break;
        default:
          break;
      }
    }

    [self processStreets];
  }

  [self addMetaField:MWMPlacePageCellTypeCoordinate];
}

- (void)addEditField
{
  [self addMetaField:MWMPlacePageCellTypeEditButton];
}

- (void)addBookmarkField
{
  [self addMetaField:MWMPlacePageCellTypeBookmark];
  [self sortMetaFields];
}

- (void)removeBookmarkField
{
  [self removeMetaField:MWMPlacePageCellTypeBookmark];
}

- (void)toggleCoordinateSystem
{
  NSUserDefaults * ud = [NSUserDefaults standardUserDefaults];
  [ud setBool:![ud boolForKey:kUserDefaultsLatLonAsDMSKey] forKey:kUserDefaultsLatLonAsDMSKey];
  [ud synchronize];
}

- (void)deserializeCuisine:(NSString *)cuisine
{
  self.cuisines = [NSSet setWithArray:[cuisine componentsSeparatedByString:kOSMCuisineSeparator]];
}

- (void)processStreets
{
  FeatureType const * feature = self.delegate.userMark->GetFeature();
  if (!feature)
    return;

  Framework & frm = GetFramework();
  auto const streets = frm.GetNearbyFeatureStreets(*feature);
  NSMutableArray * arr = [[NSMutableArray alloc] initWithCapacity:streets.size()];
  for (auto const & street : streets)
    [arr addObject:@(street.c_str())];
  self.nearbyStreets = arr;

  auto const info = frm.GetFeatureAddressInfo(*feature);
  [self setMetaField:MWMPlacePageCellTypeStreet value:info.m_street];
}

#pragma mark - Editing

- (void)setEditableTypes
{
  FeatureType * feature = self.delegate.userMark->GetFeature();
  if (!feature)
    return;

  auto & editor = osm::Editor::Instance();
  vector<Metadata::EType> const editableTypes = editor.EditableMetadataForType(*feature);
  bool const isNameEditable = editor.IsNameEditable(*feature);
  bool const isAddressEditable = editor.IsAddressEditable(*feature);
  if (!editableTypes.empty() || isAddressEditable || isNameEditable)
    [self addEditField];
  if (isNameEditable)
    m_editableFields.insert(MWMPlacePageCellTypeName);
  if (isAddressEditable)
  {
    m_editableFields.insert(MWMPlacePageCellTypeStreet);
    m_editableFields.insert(MWMPlacePageCellTypeBuilding);
  }
  for (auto const & type : editableTypes)
  {
    NSAssert(kMetaFieldsMap[type] >= Metadata::FMD_COUNT || kMetaFieldsMap[type] == 0, @"Incorrect enum value");
    MWMPlacePageCellType const field = static_cast<MWMPlacePageCellType>(kMetaFieldsMap[type]);
    m_editableFields.insert(field);
  }
}

- (BOOL)isCellEditable:(MWMPlacePageCellType)cellType
{
  return m_editableFields.count(cellType) == 1;
}

- (void)saveEditedCells:(MWMPlacePageCellTypeValueMap const &)cells
{
  FeatureType * feature = self.delegate.userMark->GetFeature();
  NSAssert(feature != nullptr, @"Feature is null");
  if (!feature)
    return;

  auto & metadata = feature->GetMetadata();
  NSString * entityStreet = [self getCellValue:MWMPlacePageCellTypeStreet];
  string streetName = (entityStreet ? entityStreet : @"").UTF8String;
  NSString * entityHouseNumber = [self getCellValue:MWMPlacePageCellTypeBuilding];
  string houseNumber = (entityHouseNumber ? entityHouseNumber : @"").UTF8String;;
  for (auto const & cell : cells)
  {
    switch (cell.first)
    {
      case MWMPlacePageCellTypePhoneNumber:
      case MWMPlacePageCellTypeWebsite:
      case MWMPlacePageCellTypeOpenHours:
      case MWMPlacePageCellTypeEmail:
      case MWMPlacePageCellTypeWiFi:
      {
        Metadata::EType const fmdType = static_cast<Metadata::EType>(kMetaFieldsMap[cell.first]);
        NSAssert(fmdType > 0 && fmdType < Metadata::FMD_COUNT, @"Incorrect enum value");
        metadata.Set(fmdType, cell.second);
        break;
      }
      case MWMPlacePageCellTypeCuisine:
      {
        Metadata::EType const fmdType = static_cast<Metadata::EType>(kMetaFieldsMap[cell.first]);
        NSAssert(fmdType > 0 && fmdType < Metadata::FMD_COUNT, @"Incorrect enum value");
        NSString * osmCuisineStr = mwmToOSMCuisineString(@(cell.second.c_str()));
        metadata.Set(fmdType, osmCuisineStr.UTF8String);
        break;
      }
      case MWMPlacePageCellTypeName:
      {
        // TODO(AlexZ): Make sure that we display and save name in the same language (default?).
        auto names = feature->GetNames();
        names.AddString(StringUtf8Multilang::DEFAULT_CODE, cell.second);
        feature->SetNames(names);
        break;
      }
      case MWMPlacePageCellTypeStreet:
      {
        streetName = cell.second;
        break;
      }
      case MWMPlacePageCellTypeBuilding:
      {
        houseNumber = cell.second;
        break;
      }
      default:
        NSAssert(false, @"Invalid field for editor");
        break;
    }
  }
  osm::Editor::Instance().EditFeature(*feature, streetName, houseNumber);
}

#pragma mark - Getters

- (NSUInteger)getCellsCount
{
  return m_fields.size();
}

- (MWMPlacePageCellType)getCellType:(NSUInteger)index
{
  NSAssert(index < [self getCellsCount], @"Invalid meta index");
  return m_fields[index];
}

- (NSString *)getCellValue:(MWMPlacePageCellType)cellType
{
  switch (cellType)
  {
    case MWMPlacePageCellTypeName:
      return self.title;
    case MWMPlacePageCellTypeCoordinate:
      return [self coordinate];
    default:
    {
      auto const it = m_values.find(cellType);
      BOOL const haveField = (it != m_values.end());
      return haveField ? @(it->second.c_str()) : nil;
    }
  }
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

#pragma mark - Properties

@synthesize cuisines = _cuisines;
- (NSSet<NSString *> *)cuisines
{
  if (!_cuisines)
    _cuisines = [NSSet set];
  return _cuisines;
}

- (void)setCuisines:(NSSet<NSString *> *)cuisines
{
  if ([_cuisines isEqualToSet:cuisines])
    return;
  _cuisines = cuisines;
  [self setMetaField:MWMPlacePageCellTypeCuisine
               value:[MWMPlacePageEntity makeMWMCuisineString:cuisines].UTF8String];
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
