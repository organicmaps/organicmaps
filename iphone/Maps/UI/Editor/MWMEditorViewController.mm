#import "MWMEditorViewController.h"
#import "MWMAlertViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMButtonCell.h"
#import "MWMCuisineEditorViewController.h"
#import "MWMDropDown.h"
#import "MWMEditorAddAdditionalNameTableViewCell.h"
#import "MWMEditorAdditionalNameTableViewCell.h"
#import "MWMEditorAdditionalNamesHeader.h"
#import "MWMEditorAdditionalNamesTableViewController.h"
#import "MWMEditorCategoryCell.h"
#import "MWMEditorCellType.h"
#import "MWMEditorCommon.h"
#import "MWMEditorNotesFooter.h"
#import "MWMEditorSelectTableViewCell.h"
#import "MWMEditorSwitchTableViewCell.h"
#import "MWMEditorTextTableViewCell.h"
#import "MWMNoteCell.h"
#import "MWMObjectsCategorySelectorController.h"
#import "MWMOpeningHoursEditorViewController.h"
#import "MWMPlacePageOpeningHoursCell.h"
#import "MWMStreetEditorViewController.h"
#import "MapViewController.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "indexer/osm_editor.hpp"

namespace
{
NSString * const kAdditionalNamesEditorSegue = @"Editor2AdditionalNamesEditorSegue";
NSString * const kOpeningHoursEditorSegue = @"Editor2OpeningHoursEditorSegue";
NSString * const kCuisineEditorSegue = @"Editor2CuisineEditorSegue";
NSString * const kStreetEditorSegue = @"Editor2StreetEditorSegue";
NSString * const kCategoryEditorSegue = @"Editor2CategoryEditorSegue";

NSString * const kUDEditorPersonalInfoWarninWasShown = @"PersonalInfoWarningAlertWasShown";

CGFloat const kDefaultHeaderHeight = 28.;
CGFloat const kDefaultFooterHeight = 32.;

typedef NS_ENUM(NSUInteger, MWMEditorSection) {
  MWMEditorSectionCategory,
  MWMEditorSectionAdditionalNames,
  MWMEditorSectionAddress,
  MWMEditorSectionDetails,
  MWMEditorSectionNote,
  MWMEditorSectionButton
};

vector<MWMEditorCellType> const kSectionCategoryCellTypes{MWMEditorCellTypeCategory};
vector<MWMEditorCellType> const kSectionAddressCellTypes{
    MWMEditorCellTypeStreet, MWMEditorCellTypeBuilding, MWMEditorCellTypeZipCode};

vector<MWMEditorCellType> const kSectionNoteCellTypes{MWMEditorCellTypeNote};
vector<MWMEditorCellType> const kSectionButtonCellTypes{MWMEditorCellTypeReportButton};

using MWMEditorCellTypeClassMap = map<MWMEditorCellType, Class>;
MWMEditorCellTypeClassMap const kCellType2Class{
    {MWMEditorCellTypeCategory, [MWMEditorCategoryCell class]},
    {MWMEditorCellTypeAdditionalName, [MWMEditorAdditionalNameTableViewCell class]},
    {MWMEditorCellTypeAddAdditionalName, [MWMEditorAddAdditionalNameTableViewCell class]},
    {MWMEditorCellTypeAddAdditionalNamePlaceholder,
     [MWMEditorAdditionalNamePlaceholderTableViewCell class]},
    {MWMEditorCellTypeStreet, [MWMEditorSelectTableViewCell class]},
    {MWMEditorCellTypeBuilding, [MWMEditorTextTableViewCell class]},
    {MWMEditorCellTypeZipCode, [MWMEditorTextTableViewCell class]},
    {MWMEditorCellTypeBuildingLevels, [MWMEditorTextTableViewCell class]},
    {MWMEditorCellTypeOpenHours, [MWMPlacePageOpeningHoursCell class]},
    {MWMEditorCellTypePhoneNumber, [MWMEditorTextTableViewCell class]},
    {MWMEditorCellTypeWebsite, [MWMEditorTextTableViewCell class]},
    {MWMEditorCellTypeEmail, [MWMEditorTextTableViewCell class]},
    {MWMEditorCellTypeOperator, [MWMEditorTextTableViewCell class]},
    {MWMEditorCellTypeCuisine, [MWMEditorSelectTableViewCell class]},
    {MWMEditorCellTypeWiFi, [MWMEditorSwitchTableViewCell class]},
    {MWMEditorCellTypeNote, [MWMNoteCell class]},
    {MWMEditorCellTypeReportButton, [MWMButtonCell class]}};

Class cellClass(MWMEditorCellType cellType)
{
  auto const it = kCellType2Class.find(cellType);
  ASSERT(it != kCellType2Class.end(), ());
  return it->second;
}

void cleanupAdditionalLanguages(vector<osm::LocalizedName> const & names,
                                vector<NSInteger> & newAdditionalLanguages)
{
  newAdditionalLanguages.erase(
      remove_if(newAdditionalLanguages.begin(), newAdditionalLanguages.end(),
                [&names](NSInteger x) {
                  auto it =
                      find_if(names.begin(), names.end(),
                              [x](osm::LocalizedName const & name) { return name.m_code == x; });
                  return it != names.end();
                }),
      newAdditionalLanguages.end());
}

vector<MWMEditorCellType> cellsForAdditionalNames(osm::NamesDataSource const & ds,
                                                  vector<NSInteger> const & newAdditionalLanguages,
                                                  BOOL showAdditionalNames)
{
  vector<MWMEditorCellType> res;
  auto const allNamesSize = ds.names.size() + newAdditionalLanguages.size();
  if (allNamesSize != 0)
  {
    if (showAdditionalNames)
    {
      res.insert(res.begin(), allNamesSize, MWMEditorCellTypeAdditionalName);
    }
    else
    {
      auto const mandatoryNamesCount = ds.mandatoryNamesCount;
      res.insert(res.begin(), mandatoryNamesCount, MWMEditorCellTypeAdditionalName);
      if (allNamesSize > mandatoryNamesCount)
        res.push_back(MWMEditorCellTypeAddAdditionalNamePlaceholder);
    }
  }
  res.push_back(MWMEditorCellTypeAddAdditionalName);
  return res;
}

vector<MWMEditorCellType> cellsForProperties(vector<osm::Props> const & props)
{
  using namespace osm;
  vector<MWMEditorCellType> res;
  for (auto const p : props)
  {
    switch (p)
    {
    case Props::OpeningHours: res.push_back(MWMEditorCellTypeOpenHours); break;
    case Props::Phone: res.push_back(MWMEditorCellTypePhoneNumber); break;
    case Props::Website: res.push_back(MWMEditorCellTypeWebsite); break;
    case Props::Email: res.push_back(MWMEditorCellTypeEmail); break;
    case Props::Cuisine: res.push_back(MWMEditorCellTypeCuisine); break;
    case Props::Operator: res.push_back(MWMEditorCellTypeOperator); break;
    case Props::Internet: res.push_back(MWMEditorCellTypeWiFi); break;
    case Props::Wikipedia:
    case Props::Fax:
    case Props::Stars:
    case Props::Elevation:
    case Props::Flats:
    case Props::BuildingLevels:
    case Props::Level: break;
    }
  }
  return res;
}

void registerCellsForTableView(vector<MWMEditorCellType> const & cells, UITableView * tv)
{
  for (auto const c : cells)
    [tv registerWithCellClass:cellClass(c)];
}
}  // namespace

@interface MWMEditorViewController ()<
    UITableViewDelegate, UITableViewDataSource, UITextFieldDelegate, MWMOpeningHoursEditorProtocol,
    MWMPlacePageOpeningHoursCellProtocol, MWMEditorCellProtocol, MWMCuisineEditorProtocol,
    MWMStreetEditorProtocol, MWMObjectsCategorySelectorDelegate, MWMNoteCelLDelegate,
    MWMEditorAdditionalName, MWMButtonCellDelegate, MWMEditorAdditionalNamesProtocol>

@property(nonatomic) NSMutableDictionary<Class, UITableViewCell *> * offscreenCells;
@property(nonatomic) NSMutableArray<NSIndexPath *> * invalidCells;
@property(nonatomic) MWMEditorAdditionalNamesHeader * additionalNamesHeader;
@property(nonatomic) MWMEditorNotesFooter * notesFooter;
@property(copy, nonatomic) NSString * note;
@property(nonatomic) osm::Editor::FeatureStatus featureStatus;
@property(nonatomic) BOOL isFeatureUploaded;

@property(nonatomic) BOOL showAdditionalNames;

@end

@implementation MWMEditorViewController
{
  vector<MWMEditorSection> m_sections;
  map<MWMEditorSection, vector<MWMEditorCellType>> m_cells;
  osm::EditableMapObject m_mapObject;
  vector<NSInteger> m_newAdditionalLanguages;
}

- (void)viewDidLoad
{
  [Statistics logEvent:kStatEventName(kStatEdit, kStatOpen)];
  [super viewDidLoad];
  [self configTable];
  [self configNavBar];
  auto const & fid = m_mapObject.GetID();
  self.featureStatus = osm::Editor::Instance().GetFeatureStatus(fid.m_mwmId, fid.m_index);
  self.isFeatureUploaded = osm::Editor::Instance().IsFeatureUploaded(fid.m_mwmId, fid.m_index);
  m_newAdditionalLanguages.clear();
}

- (void)setFeatureToEdit:(FeatureID const &)fid
{
  if (!GetFramework().GetEditableMapObject(fid, m_mapObject))
    NSAssert(false, @"Incorrect featureID.");
}

- (void)setEditableMapObject:(osm::EditableMapObject const &)emo
{
  NSAssert(self.isCreating, @"We should pass featureID to editor if we just editing");
  m_mapObject = emo;
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  [self.tableView reloadData];
}

#pragma mark - Configuration

- (void)configNavBar
{
  self.title =
      L(self.isCreating ? @"editor_add_place_title" : @"editor_edit_place_title").capitalizedString;
  self.navigationItem.rightBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemSave
                                                    target:self
                                                    action:@selector(onSave)];
}

- (void)backTap
{
  if (self.isCreating)
    [self.navigationController popToRootViewControllerAnimated:YES];
  else
    [super backTap];
}

- (void)showBackButton
{
  if (self.isCreating)
  {
    self.navigationItem.leftBarButtonItem =
        [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
                                                      target:self
                                                      action:@selector(backTap)];
  }
  else
  {
    [super showBackButton];
  }
}

#pragma mark - Actions

- (void)onSave
{
  if (![self.view endEditing:YES])
  {
    NSAssert(false, @"We can't save map object because one of text fields can't apply it's text!");
    return;
  }

  if (self.invalidCells.count)
  {
    NSIndexPath * ip = self.invalidCells.firstObject;
    MWMEditorTextTableViewCell * cell = [self.tableView cellForRowAtIndexPath:ip];
    [cell.textField becomeFirstResponder];
    return;
  }

  if ([self showPersonalInfoWarningAlertIfNeeded])
    return;

  auto & f = GetFramework();
  auto const & featureID = m_mapObject.GetID();
  NSDictionary * info = @{
    kStatEditorMWMName : @(featureID.GetMwmName().c_str()),
    kStatEditorMWMVersion : @(featureID.GetMwmVersion())
  };
  BOOL const haveNote = self.note.length;

  if (haveNote)
  {
    auto const latLon = m_mapObject.GetLatLon();
    NSMutableDictionary * noteInfo = [info mutableCopy];
    noteInfo[kStatProblem] = self.note;
    CLLocation * location = [[CLLocation alloc] initWithLatitude:latLon.lat longitude:latLon.lon];
    [Statistics logEvent:kStatEditorProblemReport withParameters:noteInfo atLocation:location];
    f.CreateNote(m_mapObject, osm::Editor::NoteProblemType::General, self.note.UTF8String);
  }

  switch (f.SaveEditedMapObject(m_mapObject))
  {
  case osm::Editor::NoUnderlyingMapError:
  case osm::Editor::SavingError:
    [self.navigationController popToRootViewControllerAnimated:YES];
    break;
  case osm::Editor::NothingWasChanged:
    [self.navigationController popToRootViewControllerAnimated:YES];
    if (haveNote)
      [self showDropDown];
    break;
  case osm::Editor::SavedSuccessfully:
    [Statistics logEvent:(self.isCreating ? kStatEditorAddSuccess : kStatEditorEditSuccess)
          withParameters:info];
    osm_auth_ios::AuthorizationSetNeedCheck(YES);
    f.UpdatePlacePageInfoForCurrentSelection();
    [self.navigationController popToRootViewControllerAnimated:YES];
    break;
  case osm::Editor::NoFreeSpaceError:
    [Statistics logEvent:(self.isCreating ? kStatEditorAddError : kStatEditorEditError)
          withParameters:info];
    [self.alertController presentNotEnoughSpaceAlert];
    break;
  }
}

- (void)showDropDown
{
  MWMDropDown * dd = [[MWMDropDown alloc] initWithSuperview:[MapViewController controller].view];
  [dd showWithMessage:L(@"editor_edits_sent_message")];
}

#pragma mark - Headers

- (MWMEditorAdditionalNamesHeader *)additionalNamesHeader
{
  if (!_additionalNamesHeader)
  {
    __weak auto weakSelf = self;
    _additionalNamesHeader = [MWMEditorAdditionalNamesHeader header:^{
      __strong auto self = weakSelf;
      self.showAdditionalNames = !self.showAdditionalNames;
    }];
  }
  return _additionalNamesHeader;
}

#pragma mark - Footers

- (MWMEditorNotesFooter *)notesFooter
{
  if (!_notesFooter)
    _notesFooter = [MWMEditorNotesFooter footerForController:self];
  return _notesFooter;
}

#pragma mark - Properties

- (void)setShowAdditionalNames:(BOOL)showAdditionalNames
{
  _showAdditionalNames = showAdditionalNames;
  [self.additionalNamesHeader setShowAdditionalNames:showAdditionalNames];
  [self configTable];
  auto const additionalNamesSectionIt =
      find(m_sections.begin(), m_sections.end(), MWMEditorSectionAdditionalNames);
  if (additionalNamesSectionIt == m_sections.end())
  {
    [self.tableView reloadData];
  }
  else
  {
    auto const sectionIndex = distance(m_sections.begin(), additionalNamesSectionIt);
    [self.tableView reloadSections:[[NSIndexSet alloc] initWithIndex:sectionIndex]
                  withRowAnimation:UITableViewRowAnimationAutomatic];
  }
}

#pragma mark - Offscreen cells

- (UITableViewCell *)offscreenCellForClass:(Class)cls
{
  auto cell = self.offscreenCells[cls];
  if (!cell)
  {
    cell = [NSBundle.mainBundle loadWithViewClass:cls owner:nil options:nil].firstObject;
    self.offscreenCells[cls] = cell;
  }
  return cell;
}

- (void)configTable
{
  self.offscreenCells = [NSMutableDictionary dictionary];
  self.invalidCells = [NSMutableArray array];
  m_sections.clear();
  m_cells.clear();

  m_sections.push_back(MWMEditorSectionCategory);
  m_cells[MWMEditorSectionCategory] = kSectionCategoryCellTypes;
  registerCellsForTableView(kSectionCategoryCellTypes, self.tableView);
  BOOL const isNameEditable = m_mapObject.IsNameEditable();
  BOOL const isAddressEditable = m_mapObject.IsAddressEditable();
  BOOL const areEditablePropertiesEmpty = m_mapObject.GetEditableProperties().empty();
  BOOL const isCreating = self.isCreating;
  BOOL const isThereNotes =
      !isCreating && areEditablePropertiesEmpty && !isAddressEditable && !isNameEditable;

  if (isNameEditable)
  {
    auto const ds = m_mapObject.GetNamesDataSource();
    auto const & localizedNames = ds.names;
    cleanupAdditionalLanguages(localizedNames, m_newAdditionalLanguages);
    auto const cells =
        cellsForAdditionalNames(ds, m_newAdditionalLanguages, self.showAdditionalNames);
    m_sections.push_back(MWMEditorSectionAdditionalNames);
    m_cells[MWMEditorSectionAdditionalNames] = cells;
    registerCellsForTableView(cells, self.tableView);
    [self.additionalNamesHeader setAdditionalNamesVisible:cells.size() > ds.mandatoryNamesCount + 1];
  }

  if (isAddressEditable)
  {
    m_sections.push_back(MWMEditorSectionAddress);
    m_cells[MWMEditorSectionAddress] = kSectionAddressCellTypes;
    if (m_mapObject.IsBuilding() && !m_mapObject.IsPointType())
      m_cells[MWMEditorSectionAddress].push_back(MWMEditorCellTypeBuildingLevels);

    registerCellsForTableView(kSectionAddressCellTypes, self.tableView);
  }

  if (!areEditablePropertiesEmpty)
  {
    auto const cells = cellsForProperties(m_mapObject.GetEditableProperties());
    if (!cells.empty())
    {
      m_sections.push_back(MWMEditorSectionDetails);
      m_cells[MWMEditorSectionDetails] = cells;
      registerCellsForTableView(cells, self.tableView);
    }
  }

  if (isThereNotes)
  {
    m_sections.push_back(MWMEditorSectionNote);
    m_cells[MWMEditorSectionNote] = kSectionNoteCellTypes;
    registerCellsForTableView(kSectionNoteCellTypes, self.tableView);
  }

  if (isCreating)
    return;
  m_sections.push_back(MWMEditorSectionButton);
  m_cells[MWMEditorSectionButton] = kSectionButtonCellTypes;
  registerCellsForTableView(kSectionButtonCellTypes, self.tableView);
}

- (MWMEditorCellType)cellTypeForIndexPath:(NSIndexPath *)indexPath
{
  return m_cells[m_sections[indexPath.section]][indexPath.row];
}

- (Class)cellClassForIndexPath:(NSIndexPath *)indexPath
{
  return cellClass([self cellTypeForIndexPath:indexPath]);
}

#pragma mark - Fill cells with data

- (void)fillCell:(UITableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  BOOL const isValid = ![self.invalidCells containsObject:indexPath];
  switch ([self cellTypeForIndexPath:indexPath])
  {
  case MWMEditorCellTypeCategory:
  {
    MWMEditorCategoryCell * cCell = static_cast<MWMEditorCategoryCell *>(cell);
    [cCell configureWithDelegate:self
                     detailTitle:@(m_mapObject.GetLocalizedType().c_str())
                      isCreating:self.isCreating];
    break;
  }
  case MWMEditorCellTypePhoneNumber:
  {
    MWMEditorTextTableViewCell * tCell = static_cast<MWMEditorTextTableViewCell *>(cell);
    [tCell configWithDelegate:self
                         icon:[UIImage imageNamed:@"ic_placepage_phone_number"]
                         text:@(m_mapObject.GetPhone().c_str())
                  placeholder:L(@"phone")
                 errorMessage:L(@"error_enter_correct_phone")
                      isValid:isValid
                 keyboardType:UIKeyboardTypeNamePhonePad
               capitalization:UITextAutocapitalizationTypeNone];
    break;
  }
  case MWMEditorCellTypeWebsite:
  {
    MWMEditorTextTableViewCell * tCell = static_cast<MWMEditorTextTableViewCell *>(cell);
    [tCell configWithDelegate:self
                         icon:[UIImage imageNamed:@"ic_placepage_website"]
                         text:@(m_mapObject.GetWebsite().c_str())
                  placeholder:L(@"website")
                 errorMessage:L(@"error_enter_correct_web")
                      isValid:isValid
                 keyboardType:UIKeyboardTypeURL
               capitalization:UITextAutocapitalizationTypeNone];
    break;
  }
  case MWMEditorCellTypeEmail:
  {
    MWMEditorTextTableViewCell * tCell = static_cast<MWMEditorTextTableViewCell *>(cell);
    [tCell configWithDelegate:self
                         icon:[UIImage imageNamed:@"ic_placepage_email"]
                         text:@(m_mapObject.GetEmail().c_str())
                  placeholder:L(@"email")
                 errorMessage:L(@"error_enter_correct_email")
                      isValid:isValid
                 keyboardType:UIKeyboardTypeEmailAddress
               capitalization:UITextAutocapitalizationTypeNone];
    break;
  }
  case MWMEditorCellTypeOperator:
  {
    MWMEditorTextTableViewCell * tCell = static_cast<MWMEditorTextTableViewCell *>(cell);
    [tCell configWithDelegate:self
                         icon:[UIImage imageNamed:@"ic_operator"]
                         text:@(m_mapObject.GetOperator().c_str())
                  placeholder:L(@"editor_operator")
                 keyboardType:UIKeyboardTypeDefault
               capitalization:UITextAutocapitalizationTypeSentences];
    break;
  }
  case MWMEditorCellTypeOpenHours:
  {
    MWMPlacePageOpeningHoursCell * tCell = static_cast<MWMPlacePageOpeningHoursCell *>(cell);
    NSString * text = @(m_mapObject.GetOpeningHours().c_str());
    [tCell configWithDelegate:self info:(text.length ? text : L(@"add_opening_hours"))];
    break;
  }
  case MWMEditorCellTypeWiFi:
  {
    MWMEditorSwitchTableViewCell * tCell = static_cast<MWMEditorSwitchTableViewCell *>(cell);
    // TODO(Vlad, IgorTomko): Support all other possible Internet statuses.
    [tCell configWithDelegate:self
                         icon:[UIImage imageNamed:@"ic_placepage_wifi"]
                         text:L(@"wifi")
                           on:m_mapObject.GetInternet() == osm::Internet::Wlan];
    break;
  }
  case MWMEditorCellTypeAdditionalName:
  {
    MWMEditorAdditionalNameTableViewCell * tCell =
        static_cast<MWMEditorAdditionalNameTableViewCell *>(cell);

    // When default name is added - remove fake names from datasource.
    auto const it = std::find(m_newAdditionalLanguages.begin(), m_newAdditionalLanguages.end(),
                              StringUtf8Multilang::kDefaultCode);
    auto const needFakes = it == m_newAdditionalLanguages.end();
    auto const & localizedNames = m_mapObject.GetNamesDataSource(needFakes).names;

    if (indexPath.row < localizedNames.size())
    {
      osm::LocalizedName const & name = localizedNames[indexPath.row];
      [tCell configWithDelegate:self
                       langCode:name.m_code
                       langName:@(name.m_langName)
                           name:@(name.m_name.c_str())
                   keyboardType:UIKeyboardTypeDefault];
    }
    else
    {
      NSInteger const newAdditionalNameIndex = indexPath.row - localizedNames.size();
      NSInteger const langCode = m_newAdditionalLanguages[newAdditionalNameIndex];

      string name;
      // Default name can be changed in advanced mode.
      if (langCode == StringUtf8Multilang::kDefaultCode)
      {
        name = m_mapObject.GetDefaultName();
        m_mapObject.EnableNamesAdvancedMode();
      }

      [tCell configWithDelegate:self
                       langCode:langCode
                       langName:@(StringUtf8Multilang::GetLangNameByCode(langCode))
                           name:@(name.c_str())
                   keyboardType:UIKeyboardTypeDefault];
    }
    break;
  }
  case MWMEditorCellTypeAddAdditionalName:
  {
    MWMEditorAddAdditionalNameTableViewCell * tCell =
        static_cast<MWMEditorAddAdditionalNameTableViewCell *>(cell);
    [tCell configWithDelegate:self];
    break;
  }
  case MWMEditorCellTypeAddAdditionalNamePlaceholder: break;
  case MWMEditorCellTypeStreet:
  {
    MWMEditorSelectTableViewCell * tCell = static_cast<MWMEditorSelectTableViewCell *>(cell);
    [tCell configWithDelegate:self
                         icon:[UIImage imageNamed:@"ic_placepage_adress"]
                         text:@(m_mapObject.GetStreet().m_defaultName.c_str())
                  placeholder:L(@"add_street")];
    break;
  }
  case MWMEditorCellTypeBuilding:
  {
    MWMEditorTextTableViewCell * tCell = static_cast<MWMEditorTextTableViewCell *>(cell);
    [tCell configWithDelegate:self
                         icon:nil
                         text:@(m_mapObject.GetHouseNumber().c_str())
                  placeholder:L(@"house_number")
                 errorMessage:L(@"error_enter_correct_house_number")
                      isValid:isValid
                 keyboardType:UIKeyboardTypeDefault
               capitalization:UITextAutocapitalizationTypeNone];
    break;
  }
  case MWMEditorCellTypeZipCode:
  {
    MWMEditorTextTableViewCell * tCell = static_cast<MWMEditorTextTableViewCell *>(cell);
    [tCell configWithDelegate:self
                         icon:nil
                         text:@(m_mapObject.GetPostcode().c_str())
                  placeholder:L(@"editor_zip_code")
                 errorMessage:L(@"error_enter_correct_zip_code")
                      isValid:isValid
                 keyboardType:UIKeyboardTypeDefault
               capitalization:UITextAutocapitalizationTypeAllCharacters];
    break;
  }
  case MWMEditorCellTypeBuildingLevels:
  {
    NSString * placeholder =
        [NSString stringWithFormat:L(@"editor_storey_number"),
                                   osm::EditableMapObject::kMaximumLevelsEditableByUsers];
    NSString * errorMessage =
        [NSString stringWithFormat:L(@"error_enter_correct_storey_number"),
                                   osm::EditableMapObject::kMaximumLevelsEditableByUsers];
    MWMEditorTextTableViewCell * tCell = static_cast<MWMEditorTextTableViewCell *>(cell);
    [tCell configWithDelegate:self
                         icon:nil
                         text:@(m_mapObject.GetBuildingLevels().c_str())
                  placeholder:placeholder
                 errorMessage:errorMessage
                      isValid:isValid
                 keyboardType:UIKeyboardTypeNumberPad
               capitalization:UITextAutocapitalizationTypeNone];
    break;
  }
  case MWMEditorCellTypeCuisine:
  {
    MWMEditorSelectTableViewCell * tCell = static_cast<MWMEditorSelectTableViewCell *>(cell);
    [tCell configWithDelegate:self
                         icon:[UIImage imageNamed:@"ic_placepage_cuisine"]
                         text:@(m_mapObject.FormatCuisines().c_str())
                  placeholder:L(@"select_cuisine")];
    break;
  }
  case MWMEditorCellTypeNote:
  {
    MWMNoteCell * tCell = static_cast<MWMNoteCell *>(cell);
    [tCell configWithDelegate:self
                     noteText:self.note
                  placeholder:L(@"editor_detailed_description_hint")];
    break;
  }
  case MWMEditorCellTypeReportButton:
  {
    MWMButtonCell * tCell = static_cast<MWMButtonCell *>(cell);

    auto title = ^NSString *(osm::Editor::FeatureStatus s, BOOL isUploaded)
    {
      if (isUploaded)
        return L(@"editor_place_doesnt_exist");
      switch (s)
      {
      case osm::Editor::FeatureStatus::Untouched: return L(@"editor_place_doesnt_exist");
      case osm::Editor::FeatureStatus::Deleted:
      case osm::Editor::FeatureStatus::Obsolete:  // TODO(Vlad): Either make a valid button or
                                                  // disable it.
        NSAssert(false, @"Incorrect feature status!");
        return L(@"editor_place_doesnt_exist");
      case osm::Editor::FeatureStatus::Modified: return L(@"editor_reset_edits_button");
      case osm::Editor::FeatureStatus::Created: return L(@"editor_remove_place_button");
      }
    };

    [tCell configureWithDelegate:self title:title(self.featureStatus, self.isFeatureUploaded)];
    break;
  }
  default: NSAssert(false, @"Invalid field for editor"); break;
  }
}

#pragma mark - UITableViewDataSource

- (UITableViewCell * _Nonnull)tableView:(UITableView * _Nonnull)tableView
                  cellForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  Class cls = [self cellClassForIndexPath:indexPath];
  auto cell = [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath];
  [self fillCell:cell atIndexPath:indexPath];
  return cell;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView * _Nonnull)tableView
{
  return m_sections.size();
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  return m_cells[m_sections[section]].size();
}

#pragma mark - UITableViewDelegate

- (CGFloat)tableView:(UITableView * _Nonnull)tableView
    heightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  Class cls = [self cellClassForIndexPath:indexPath];
  auto cell = [self offscreenCellForClass:cls];
  [self fillCell:cell atIndexPath:indexPath];
  MWMEditorCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
  case MWMEditorCellTypeOpenHours: return ((MWMPlacePageOpeningHoursCell *)cell).cellHeight;
  case MWMEditorCellTypeCategory:
  case MWMEditorCellTypeReportButton: return self.tableView.rowHeight;
  case MWMEditorCellTypeNote: return static_cast<MWMNoteCell *>(cell).cellHeight;
  default:
  {
    [cell setNeedsUpdateConstraints];
    [cell updateConstraintsIfNeeded];
    cell.bounds = {{}, {CGRectGetWidth(tableView.bounds), CGRectGetHeight(cell.bounds)}};
    [cell setNeedsLayout];
    [cell layoutIfNeeded];
    CGSize const size =
        [cell.contentView systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
    return size.height;
  }
  }
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  switch (m_sections[section])
  {
  case MWMEditorSectionAdditionalNames:
  case MWMEditorSectionCategory:
  case MWMEditorSectionButton: return nil;
  case MWMEditorSectionNote: return L(@"editor_other_info");
  case MWMEditorSectionAddress: return L(@"address");
  case MWMEditorSectionDetails: return L(@"details");
  }
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
  switch (m_sections[section])
  {
  case MWMEditorSectionAdditionalNames: return self.additionalNamesHeader;
  case MWMEditorSectionCategory:
  case MWMEditorSectionButton:
  case MWMEditorSectionNote:
  case MWMEditorSectionAddress:
  case MWMEditorSectionDetails: return nil;
  }
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section
{
  switch (m_sections[section])
  {
  case MWMEditorSectionAddress:
  case MWMEditorSectionCategory:
  case MWMEditorSectionAdditionalNames:
  case MWMEditorSectionButton: return nil;
  case MWMEditorSectionDetails:
    if (find(m_sections.begin(), m_sections.end(), MWMEditorSectionNote) == m_sections.end())
      return self.notesFooter;
    return nil;
  case MWMEditorSectionNote: return self.notesFooter;
  }
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section
{
  return kDefaultHeaderHeight;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section
{
  switch (m_sections[section])
  {
  case MWMEditorSectionAddress:
      return 1.0;
  case MWMEditorSectionDetails:
    if (find(m_sections.begin(), m_sections.end(), MWMEditorSectionNote) == m_sections.end())
      return self.notesFooter.height;
    return 1.0;
  case MWMEditorSectionNote: return self.notesFooter.height;
  case MWMEditorSectionCategory:
  case MWMEditorSectionAdditionalNames:
  case MWMEditorSectionButton: return kDefaultFooterHeight;
  }
}

#pragma mark - MWMPlacePageOpeningHoursCellProtocol

- (BOOL)forcedButton { return YES; }
- (BOOL)isPlaceholder { return m_mapObject.GetOpeningHours().empty(); }
- (BOOL)isEditor { return YES; }
- (BOOL)openingHoursCellExpanded { return YES; }
- (void)setOpeningHoursCellExpanded:(BOOL)openingHoursCellExpanded
{
  [self performSegueWithIdentifier:kOpeningHoursEditorSegue sender:nil];
}

- (void)markCellAsInvalid:(NSIndexPath *)indexPath
{
  if (![self.invalidCells containsObject:indexPath])
    [self.invalidCells addObject:indexPath];

  [self.tableView reloadRowsAtIndexPaths:@[ indexPath ]
                        withRowAnimation:UITableViewRowAnimationFade];
}

#pragma mark - MWMNoteCellDelegate

- (void)cellShouldChangeSize:(MWMNoteCell *)cell text:(NSString *)text
{
  self.offscreenCells[cellClass(MWMEditorCellTypeNote)] = cell;
  self.note = text;
  [self.tableView refresh];
  NSIndexPath * ip = [self.tableView indexPathForCell:cell];
  [self.tableView scrollToRowAtIndexPath:ip
                        atScrollPosition:UITableViewScrollPositionBottom
                                animated:YES];
}

- (void)cell:(MWMNoteCell *)cell didFinishEditingWithText:(NSString *)text { self.note = text; }
#pragma mark - MWMEditorAdditionalName

- (void)editAdditionalNameLanguage:(NSInteger)selectedLangCode
{
  [self performSegueWithIdentifier:kAdditionalNamesEditorSegue sender:@(selectedLangCode)];
}

#pragma mark - MWMEditorAdditionalNamesProtocol

- (void)addAdditionalName:(NSInteger)languageIndex
{
  m_newAdditionalLanguages.push_back(languageIndex);
  self.showAdditionalNames = YES;
  auto additionalNamesSectionIt =
      find(m_sections.begin(), m_sections.end(), MWMEditorSectionAdditionalNames);
  assert(additionalNamesSectionIt != m_sections.end());
  auto const section = distance(m_sections.begin(), additionalNamesSectionIt);
  NSInteger const row = [self tableView:self.tableView numberOfRowsInSection:section];
  assert(row > 0);
  NSIndexPath * indexPath = [NSIndexPath indexPathForRow:row - 1 inSection:section];
  [self.tableView scrollToRowAtIndexPath:indexPath
                        atScrollPosition:UITableViewScrollPositionMiddle
                                animated:NO];
}

#pragma mark - MWMEditorCellProtocol

- (void)tryToChangeInvalidStateForCell:(MWMEditorTextTableViewCell *)cell
{
  [self.tableView update:^{
    NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
    [self.invalidCells removeObject:indexPath];
  }];
}

- (void)cell:(MWMTableViewCell *)cell changedText:(NSString *)changeText
{
  NSAssert(changeText != nil, @"String can't be nil!");
  NSIndexPath * indexPath = [self.tableView indexPathForRowAtPoint:cell.center];
  MWMEditorCellType const cellType = [self cellTypeForIndexPath:indexPath];
  string const val = changeText.UTF8String;
  switch (cellType)
  {
  case MWMEditorCellTypePhoneNumber:
    m_mapObject.SetPhone(val);
    if (!osm::EditableMapObject::ValidatePhone(val))
      [self markCellAsInvalid:indexPath];
    break;
  case MWMEditorCellTypeWebsite:
    m_mapObject.SetWebsite(val);
    if (!osm::EditableMapObject::ValidateWebsite(val))
      [self markCellAsInvalid:indexPath];
    break;
  case MWMEditorCellTypeEmail:
    m_mapObject.SetEmail(val);
    if (!osm::EditableMapObject::ValidateEmail(val))
      [self markCellAsInvalid:indexPath];
    break;
  case MWMEditorCellTypeOperator: m_mapObject.SetOperator(val); break;
  case MWMEditorCellTypeBuilding:
    m_mapObject.SetHouseNumber(val);
    if (!osm::EditableMapObject::ValidateHouseNumber(val))
      [self markCellAsInvalid:indexPath];
    break;
  case MWMEditorCellTypeZipCode:
    m_mapObject.SetPostcode(val);
    if (!osm::EditableMapObject::ValidatePostCode(val))
      [self markCellAsInvalid:indexPath];
    break;
  case MWMEditorCellTypeBuildingLevels:
    m_mapObject.SetBuildingLevels(val);
    if (!osm::EditableMapObject::ValidateBuildingLevels(val))
      [self markCellAsInvalid:indexPath];
    break;
  case MWMEditorCellTypeAdditionalName:
  {
    MWMEditorAdditionalNameTableViewCell * tCell =
        static_cast<MWMEditorAdditionalNameTableViewCell *>(cell);
    m_mapObject.SetName(val, tCell.code);
    break;
  }
  default: NSAssert(false, @"Invalid field for changeText");
  }
}

- (void)cell:(UITableViewCell *)cell changeSwitch:(BOOL)changeSwitch
{
  NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
  MWMEditorCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
  case MWMEditorCellTypeWiFi:
    m_mapObject.SetInternet(changeSwitch ? osm::Internet::Wlan : osm::Internet::Unknown);
    break;
  default: NSAssert(false, @"Invalid field for changeSwitch"); break;
  }
}

#pragma mark - MWMEditorCellProtocol && MWMButtonCellDelegate

- (void)cellSelect:(UITableViewCell *)cell
{
  NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
  MWMEditorCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
  case MWMEditorCellTypeStreet:
    [self performSegueWithIdentifier:kStreetEditorSegue sender:nil];
    break;
  case MWMEditorCellTypeCuisine:
    [self performSegueWithIdentifier:kCuisineEditorSegue sender:nil];
    break;
  case MWMEditorCellTypeCategory:
    [self performSegueWithIdentifier:kCategoryEditorSegue sender:nil];
    break;
  case MWMEditorCellTypeReportButton: [self tapOnButtonCell:cell]; break;
  default: NSAssert(false, @"Invalid field for cellSelect"); break;
  }
}

- (void)tapOnButtonCell:(UITableViewCell *)cell
{
  auto const & fid = m_mapObject.GetID();
  auto const latLon = m_mapObject.GetLatLon();
  CLLocation * location = [[CLLocation alloc] initWithLatitude:latLon.lat longitude:latLon.lon];
  self.isFeatureUploaded = osm::Editor::Instance().IsFeatureUploaded(fid.m_mwmId, fid.m_index);
  NSIndexPath * ip = [self.tableView indexPathForCell:cell];
  [self.tableView reloadRowsAtIndexPaths:@[ ip ] withRowAnimation:UITableViewRowAnimationFade];

  auto placeDoesntExistAction = ^{
    [self.alertController presentPlaceDoesntExistAlertWithBlock:^(NSString * additionalMessage) {
      string const additional = additionalMessage.length ? additionalMessage.UTF8String : "";
      [Statistics logEvent:kStatEditorProblemReport
            withParameters:@{
              kStatEditorMWMName : @(fid.GetMwmName().c_str()),
              kStatEditorMWMVersion : @(fid.GetMwmVersion()),
              kStatProblem : @(osm::Editor::kPlaceDoesNotExistMessage)
            }
                atLocation:location];
      GetFramework().CreateNote(self->m_mapObject, osm::Editor::NoteProblemType::PlaceDoesNotExist,
                                additional);
      [self backTap];
      [self showDropDown];
    }];
  };

  auto revertAction = ^(BOOL isCreated) {
    [Statistics logEvent:isCreated ? kStatEditorAddCancel : kStatEditorEditCancel
          withParameters:@{
            kStatEditorMWMName : @(fid.GetMwmName().c_str()),
            kStatEditorMWMVersion : @(fid.GetMwmVersion())
          }
              atLocation:location];
    auto & f = GetFramework();
    if (!f.RollBackChanges(fid))
      NSAssert(false, @"We shouldn't call this if we can't roll back!");

    f.UpdateUserViewportChanged();
    [self backTap];
  };

  if (self.isFeatureUploaded)
  {
    placeDoesntExistAction();
  }
  else
  {
    switch (self.featureStatus)
    {
    case osm::Editor::FeatureStatus::Untouched: placeDoesntExistAction(); break;
    case osm::Editor::FeatureStatus::Modified:
    {
      [self.alertController presentResetChangesAlertWithBlock:^{
        revertAction(NO);
      }];
      break;
    }
    case osm::Editor::FeatureStatus::Created:
    {
      [self.alertController presentDeleteFeatureAlertWithBlock:^{
        revertAction(YES);
      }];
      break;
    }
    case osm::Editor::FeatureStatus::Deleted: break;
    case osm::Editor::FeatureStatus::Obsolete: break;
    }
  }
}

#pragma mark - MWMOpeningHoursEditorProtocol

- (void)setOpeningHours:(NSString *)openingHours
{
  m_mapObject.SetOpeningHours(openingHours.UTF8String);
}

#pragma mark - MWMObjectsCategorySelectorDelegate

- (void)reloadObject:(osm::EditableMapObject const &)object
{
  [self setEditableMapObject:object];
  [self configTable];
}

#pragma mark - MWMCuisineEditorProtocol

- (vector<string>)selectedCuisines { return m_mapObject.GetCuisines(); }
- (void)setSelectedCuisines:(vector<string> const &)cuisines { m_mapObject.SetCuisines(cuisines); }
#pragma mark - MWMStreetEditorProtocol

- (void)setNearbyStreet:(osm::LocalizedStreet const &)street { m_mapObject.SetStreet(street); }
- (osm::LocalizedStreet const &)currentStreet { return m_mapObject.GetStreet(); }
- (vector<osm::LocalizedStreet> const &)nearbyStreets { return m_mapObject.GetNearbyStreets(); }
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
  else if ([segue.identifier isEqualToString:kCategoryEditorSegue])
  {
    NSAssert(self.isCreating, @"Invalid state! We'll be able to change feature category only if we "
                              @"are creating feature!");
    MWMObjectsCategorySelectorController * dvc = segue.destinationViewController;
    dvc.delegate = self;
    [dvc setSelectedCategory:m_mapObject.GetLocalizedType()];
  }
  else if ([segue.identifier isEqualToString:kAdditionalNamesEditorSegue])
  {
    MWMEditorAdditionalNamesTableViewController * dvc = segue.destinationViewController;
    [dvc configWithDelegate:self
                               name:m_mapObject.GetName()
        additionalSkipLanguageCodes:m_newAdditionalLanguages
               selectedLanguageCode:((NSNumber *)sender).integerValue];
  }
}

#pragma mark - Alert

- (BOOL)showPersonalInfoWarningAlertIfNeeded
{
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  if ([ud boolForKey:kUDEditorPersonalInfoWarninWasShown])
    return NO;

  [self.alertController presentPersonalInfoWarningAlertWithBlock:^
  {
    [ud setBool:YES forKey:kUDEditorPersonalInfoWarninWasShown];
    [ud synchronize];
    [self onSave];
  }];
  
  return YES;
}

@end
