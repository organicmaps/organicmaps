#import "MWMAuthorizationCommon.h"
#import "MWMCuisineEditorViewController.h"
#import "MWMEditorCommon.h"
#import "MWMEditorSelectTableViewCell.h"
#import "MWMEditorSwitchTableViewCell.h"
#import "MWMEditorTextTableViewCell.h"
#import "MWMEditorViewController.h"
#import "MWMOpeningHoursEditorViewController.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageOpeningHoursCell.h"
#import "MWMStreetEditorViewController.h"
#import "Statistics.h"

#include "indexer/editable_map_object.hpp"
#include "std/algorithm.hpp"

namespace
{
NSString * const kOpeningHoursEditorSegue = @"Editor2OpeningHoursEditorSegue";
NSString * const kCuisineEditorSegue = @"Editor2CuisineEditorSegue";
NSString * const kStreetEditorSegue = @"Editor2StreetEditorSegue";

typedef NS_ENUM(NSUInteger, MWMEditorSection)
{
  MWMEditorSectionName,
  MWMEditorSectionAddress,
  MWMEditorSectionDetails
};

vector<MWMPlacePageCellType> const gSectionNameCellTypes{MWMPlacePageCellTypeName};

vector<MWMPlacePageCellType> const gSectionAddressCellTypes{
    {MWMPlacePageCellTypeStreet, MWMPlacePageCellTypeBuilding}};

vector<MWMPlacePageCellType> const gSectionDetailsCellTypes{
    {MWMPlacePageCellTypeOpenHours, MWMPlacePageCellTypePhoneNumber, MWMPlacePageCellTypeWebsite,
     MWMPlacePageCellTypeEmail, MWMPlacePageCellTypeCuisine, MWMPlacePageCellTypeWiFi}};

using CellTypesSectionMap = pair<vector<MWMPlacePageCellType>, MWMEditorSection>;

vector<CellTypesSectionMap> const gCellTypesSectionMap{
    {gSectionNameCellTypes, MWMEditorSectionName},
    {gSectionAddressCellTypes, MWMEditorSectionAddress},
    {gSectionDetailsCellTypes, MWMEditorSectionDetails}};

MWMPlacePageCellTypeValueMap const gCellType2ReuseIdentifier{
    {MWMPlacePageCellTypeName, "MWMEditorNameTableViewCell"},
    {MWMPlacePageCellTypeStreet, "MWMEditorSelectTableViewCell"},
    {MWMPlacePageCellTypeBuilding, "MWMEditorTextTableViewCell"},
    {MWMPlacePageCellTypeOpenHours, "MWMPlacePageOpeningHoursCell"},
    {MWMPlacePageCellTypePhoneNumber, "MWMEditorTextTableViewCell"},
    {MWMPlacePageCellTypeWebsite, "MWMEditorTextTableViewCell"},
    {MWMPlacePageCellTypeEmail, "MWMEditorTextTableViewCell"},
    {MWMPlacePageCellTypeCuisine, "MWMEditorSelectTableViewCell"},
    {MWMPlacePageCellTypeWiFi, "MWMEditorSwitchTableViewCell"}};

NSString * reuseIdentifier(MWMPlacePageCellType cellType)
{
  auto const it = gCellType2ReuseIdentifier.find(cellType);
  BOOL const haveCell = (it != gCellType2ReuseIdentifier.end());
  ASSERT(haveCell, ());
  return haveCell ? @(it->second.c_str()) : @"";
}
} // namespace

@interface MWMEditorViewController() <UITableViewDelegate, UITableViewDataSource,
                                      UITextFieldDelegate, MWMOpeningHoursEditorProtocol,
                                      MWMPlacePageOpeningHoursCellProtocol,
                                      MWMEditorCellProtocol, MWMCuisineEditorProtocol,
                                      MWMStreetEditorProtocol>

@property (nonatomic) NSMutableDictionary<NSString *, UITableViewCell *> * offscreenCells;

@end

@implementation MWMEditorViewController
{
  vector<MWMEditorSection> m_sections;
  map<MWMEditorSection, vector<MWMPlacePageCellType>> m_cells;
  osm::EditableMapObject m_mapObject;
}

- (void)viewDidLoad
{
  [[Statistics instance] logEvent:kStatEventName(kStatEdit, kStatOpen)];
  [super viewDidLoad];
  [self configTable];
  [self configNavBar];
}

- (void)setFeatureToEdit:(FeatureID const &)fid
{
  if (!GetFramework().GetEditableMapObject(fid, m_mapObject))
    NSAssert(false, @"Incorrect featureID.");
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [self.tableView reloadData];
}

#pragma mark - Configuration

- (void)configNavBar
{
  self.title = L(@"edit_place").capitalizedString;
  self.navigationItem.leftBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
                                                    target:self
                                                    action:@selector(onCancel)];
  self.navigationItem.rightBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemSave
                                                    target:self
                                                    action:@selector(onSave)];
  self.navigationController.navigationBar.barStyle = UIBarStyleBlack;
}

#pragma mark - Actions

- (void)onCancel
{
  UINavigationController * parentNavController = self.navigationController.navigationController;
  if (parentNavController)
    [parentNavController popViewControllerAnimated:YES];
  else
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (void)onSave
{
  switch (GetFramework().SaveEditedMapObject(m_mapObject))
  {
    case osm::Editor::NothingWasChanged:
      break;
    case osm::Editor::SavedSuccessfully:
      [[Statistics instance] logEvent:kStatEventName(kStatEdit, kStatSave)];
      osm_auth_ios::AuthorizationSetNeedCheck(YES);
      break;
    case osm::Editor::NoFreeSpaceError:
      // TODO(Vlad): Show error dialog.
      break;
  }
  [self onCancel];
}

#pragma mark - Offscreen cells

- (UITableViewCell *)offscreenCellForIdentifier:(NSString *)reuseIdentifier
{
  UITableViewCell * cell = self.offscreenCells[reuseIdentifier];
  if (!cell)
  {
    cell = [[[NSBundle mainBundle] loadNibNamed:reuseIdentifier owner:nil options:nil] firstObject];
    self.offscreenCells[reuseIdentifier] = cell;
  }
  return cell;
}

// TODO(Vlad): This code can be much better.
- (bool)hasField:(feature::Metadata::EType)field
{
  auto const & editable = m_mapObject.GetEditableFields();
  return editable.end() != find(editable.begin(), editable.end(), field);
}
// TODO(Vlad): This code can be much better.
- (bool)isEditable:(MWMPlacePageCellType)cellType
{
  switch (cellType)
  {
    case MWMPlacePageCellTypePostcode: return [self hasField:feature::Metadata::FMD_POSTCODE];
    case MWMPlacePageCellTypePhoneNumber: return [self hasField:feature::Metadata::FMD_PHONE_NUMBER];
    case MWMPlacePageCellTypeWebsite: return [self hasField:feature::Metadata::FMD_WEBSITE];
    // TODO(Vlad): We should not have URL field in the UI. Website should always be used instead.
    case MWMPlacePageCellTypeURL: return [self hasField:feature::Metadata::FMD_URL];
    case MWMPlacePageCellTypeEmail: return [self hasField:feature::Metadata::FMD_EMAIL];
    case MWMPlacePageCellTypeOpenHours: return [self hasField:feature::Metadata::FMD_OPEN_HOURS];
    case MWMPlacePageCellTypeWiFi: return [self hasField:feature::Metadata::FMD_INTERNET];
    case MWMPlacePageCellTypeCuisine: return [self hasField:feature::Metadata::FMD_CUISINE];
    // TODO(Vlad): return true when we allow coordinates editing.
    case MWMPlacePageCellTypeCoordinate: return false;
    case MWMPlacePageCellTypeName: return m_mapObject.IsNameEditable();
    case MWMPlacePageCellTypeStreet: return m_mapObject.IsAddressEditable();
    case MWMPlacePageCellTypeBuilding: return m_mapObject.IsAddressEditable();
    default: NSAssert(false, @"Invalid cell type %d", cellType);
  }
  return false;
}

- (void)configTable
{
  self.offscreenCells = [NSMutableDictionary dictionary];
  m_sections.clear();
  m_cells.clear();
  for (auto const & cellsSection : gCellTypesSectionMap)
  {
    for (auto cellType : cellsSection.first)
    {
      if (![self isEditable:cellType])
        continue;
      m_sections.emplace_back(cellsSection.second);
      m_cells[cellsSection.second].emplace_back(cellType);
      NSString * identifier = reuseIdentifier(cellType);
      [self.tableView registerNib:[UINib nibWithNibName:identifier bundle:nil]
           forCellReuseIdentifier:identifier];
    }
  }
  sort(m_sections.begin(), m_sections.end());
  m_sections.erase(unique(m_sections.begin(), m_sections.end()), m_sections.end());
}

- (MWMPlacePageCellType)cellTypeForIndexPath:(NSIndexPath *)indexPath
{
  MWMEditorSection const section = m_sections[indexPath.section];
  MWMPlacePageCellType const cellType = m_cells[section][indexPath.row];
  return cellType;
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  return reuseIdentifier(cellType);
}

#pragma mark - Fill cells with data

- (void)fillCell:(UITableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case MWMPlacePageCellTypePhoneNumber:
    {
      MWMEditorTextTableViewCell * tCell = (MWMEditorTextTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_phone_number"]
                           text:@(m_mapObject.GetPhone().c_str())
                    placeholder:L(@"phone")
                   keyboardType:UIKeyboardTypePhonePad];
      break;
    }
    case MWMPlacePageCellTypeWebsite:
    {
      MWMEditorTextTableViewCell * tCell = (MWMEditorTextTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_website"]
                           text:@(m_mapObject.GetWebsite().c_str())
                    placeholder:L(@"website")
                   keyboardType:UIKeyboardTypeURL];
      break;
    }
    case MWMPlacePageCellTypeEmail:
    {
      MWMEditorTextTableViewCell * tCell = (MWMEditorTextTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_email"]
                           text:@(m_mapObject.GetEmail().c_str())
                    placeholder:L(@"email")
                   keyboardType:UIKeyboardTypeEmailAddress];
      break;
    }
    case MWMPlacePageCellTypeOpenHours:
    {
      MWMPlacePageOpeningHoursCell * tCell = (MWMPlacePageOpeningHoursCell *)cell;
      NSString * text = @(m_mapObject.GetOpeningHours().c_str());
      [tCell configWithDelegate:self info:(text.length ? text : L(@"add_opening_hours"))];
      break;
    }
    case MWMPlacePageCellTypeWiFi:
    {
      MWMEditorSwitchTableViewCell * tCell = (MWMEditorSwitchTableViewCell *)cell;
      // TODO(Vlad, IgorTomko): Support all other possible Internet statuses.
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_wifi"]
                           text:L(@"wifi")
                             on:m_mapObject.GetInternet() == osm::Internet::Wlan];
      break;
    }
    case MWMPlacePageCellTypeName:
    {
      MWMEditorTextTableViewCell * tCell = (MWMEditorTextTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:nil
                           text:@(m_mapObject.GetDefaultName().c_str())
                    placeholder:L(@"name")
                   keyboardType:UIKeyboardTypeDefault];
      break;
    }
    case MWMPlacePageCellTypeStreet:
    {
      MWMEditorSelectTableViewCell * tCell = (MWMEditorSelectTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_adress"]
                           text:@(m_mapObject.GetStreet().c_str())
                    placeholder:L(@"add_street")];
      break;
    }
    case MWMPlacePageCellTypeBuilding:
    {
      MWMEditorTextTableViewCell * tCell = (MWMEditorTextTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:nil
                           text:@(m_mapObject.GetHouseNumber().c_str())
                    placeholder:L(@"house")
                   keyboardType:UIKeyboardTypeDefault];
      break;
    }
    case MWMPlacePageCellTypeCuisine:
    {
      MWMEditorSelectTableViewCell * tCell = (MWMEditorSelectTableViewCell *)cell;
//      NSString * text = [entityValue capitalizedStringWithLocale:[NSLocale currentLocale]];
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_cuisine"]
                           text:@(m_mapObject.FormatCuisines().c_str())
                    placeholder:L(@"select_cuisine")];
      break;
    }
    default:
      NSAssert(false, @"Invalid field for editor");
      break;
  }
}

#pragma mark - UITableViewDataSource

- (UITableViewCell * _Nonnull)tableView:(UITableView * _Nonnull)tableView cellForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSString * reuseIdentifier = [self cellIdentifierForIndexPath:indexPath];
  return [tableView dequeueReusableCellWithIdentifier:reuseIdentifier];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView * _Nonnull)tableView
{
  return m_sections.size();
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  MWMEditorSection const sec = m_sections[section];
  return m_cells[sec].size();
}

#pragma mark - UITableViewDelegate

- (CGFloat)tableView:(UITableView * _Nonnull)tableView heightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSString * reuseIdentifier = [self cellIdentifierForIndexPath:indexPath];
  UITableViewCell * cell = [self offscreenCellForIdentifier:reuseIdentifier];
  // TODO(Vlad, IGrechuhin): It's bad idea to fill cells here.
  // heightForRowAtIndexPath is called way too often for the table.
  [self fillCell:cell atIndexPath:indexPath];
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case MWMPlacePageCellTypeOpenHours:
      return ((MWMPlacePageOpeningHoursCell *)cell).cellHeight;
    default:
    {
      [cell setNeedsUpdateConstraints];
      [cell updateConstraintsIfNeeded];
      cell.bounds = {{}, {CGRectGetWidth(tableView.bounds), CGRectGetHeight(cell.bounds)}};
      [cell setNeedsLayout];
      [cell layoutIfNeeded];
      CGSize const size = [cell.contentView systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
      return size.height;
    }
  }
}

- (void)tableView:(UITableView * _Nonnull)tableView willDisplayCell:(UITableViewCell * _Nonnull)cell forRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  [self fillCell:cell atIndexPath:indexPath];
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  switch (m_sections[section])
  {
  case MWMEditorSectionName:
    return nil;
  case MWMEditorSectionAddress:
    return L(@"address");
  case MWMEditorSectionDetails:
    return L(@"details");
  }
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
  switch (m_sections[section])
  {
  case MWMEditorSectionName:
    return L(@"place_name_caption");
  case MWMEditorSectionAddress:
  case MWMEditorSectionDetails:
    return nil;
  }
}

#pragma mark - MWMPlacePageOpeningHoursCellProtocol

- (BOOL)forcedButton
{
  return YES;
}

- (BOOL)isPlaceholder
{
  return m_mapObject.GetOpeningHours().empty();
}

- (BOOL)isEditor
{
  return YES;
}

- (BOOL)openingHoursCellExpanded
{
  return YES;
}

- (void)setOpeningHoursCellExpanded:(BOOL)openingHoursCellExpanded forCell:(UITableViewCell *)cell
{
  [self performSegueWithIdentifier:kOpeningHoursEditorSegue sender:nil];
}

#pragma mark - MWMEditorCellProtocol

- (void)cellBeginEditing:(UITableViewCell *)cell
{
}

- (void)cell:(UITableViewCell *)cell changeText:(NSString *)changeText
{
  NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  string const val = changeText.UTF8String;
  switch (cellType)
  {
    // TODO(Vlad): Support multilanguage names.
    case MWMPlacePageCellTypeName: m_mapObject.SetName(val, StringUtf8Multilang::kDefaultCode); break;
    case MWMPlacePageCellTypePhoneNumber: m_mapObject.SetPhone(val); break;
    case MWMPlacePageCellTypeWebsite: m_mapObject.SetWebsite(val); break;
    case MWMPlacePageCellTypeEmail: m_mapObject.SetEmail(val); break;
    case MWMPlacePageCellTypeBuilding: m_mapObject.SetHouseNumber(val); break;
    default: NSAssert(false, @"Invalid field for changeText");
  }
}

- (void)cell:(UITableViewCell *)cell changeSwitch:(BOOL)changeSwitch
{
  NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case MWMPlacePageCellTypeWiFi:
      m_mapObject.SetInternet(changeSwitch ? osm::Internet::Wlan : osm::Internet::Unknown);
      break;
    default:
      NSAssert(false, @"Invalid field for changeSwitch");
      break;
  }
}

- (void)cellSelect:(UITableViewCell *)cell
{
  NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case MWMPlacePageCellTypeStreet:
      [self performSegueWithIdentifier:kStreetEditorSegue sender:nil];
      break;
    case MWMPlacePageCellTypeCuisine:
      [self performSegueWithIdentifier:kCuisineEditorSegue sender:nil];
      break;
    default:
      NSAssert(false, @"Invalid field for cellSelect");
      break;
  }
}

#pragma mark - MWMOpeningHoursEditorProtocol

- (void)setOpeningHours:(NSString *)openingHours
{
  m_mapObject.SetOpeningHours(openingHours.UTF8String);
}

#pragma mark - MWMCuisineEditorProtocol

- (vector<string>)getSelectedCuisines
{
  return m_mapObject.GetCuisines();
}

- (void)setSelectedCuisines:(vector<string> const &)cuisines
{
  m_mapObject.SetCuisines(cuisines);
}

#pragma mark - MWMStreetEditorProtocol

- (NSString *)getStreet
{
  return @(m_mapObject.GetStreet().c_str());
}

- (void)setStreet:(NSString *)street
{
  m_mapObject.SetStreet(street.UTF8String);
}

- (NSArray<NSString *> *)getNearbyStreets
{
  auto const & streets = m_mapObject.GetNearbyStreets();
  NSMutableArray * arr = [[NSMutableArray alloc] initWithCapacity:streets.size()];
  for (auto const & street : streets)
    [arr addObject:@(street.c_str())];
  return arr;
}

#pragma mark - Segue

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
  if ([segue.identifier isEqualToString:kOpeningHoursEditorSegue])
  {
    MWMOpeningHoursEditorViewController * dvc = segue.destinationViewController;
    dvc.openingHours = @(m_mapObject.GetOpeningHours().c_str());
    dvc.delegate = self;
  }
  else if ([segue.identifier isEqualToString:kCuisineEditorSegue])
  {
    MWMCuisineEditorViewController * dvc = segue.destinationViewController;
    dvc.delegate = self;
  }
  else if ([segue.identifier isEqualToString:kStreetEditorSegue])
  {
    MWMStreetEditorViewController * dvc = segue.destinationViewController;
    dvc.delegate = self;
  }
}

@end
