#import "MapsAppDelegate.h"
#import "MWMAlertViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMButtonCell.h"
#import "MWMCuisineEditorViewController.h"
#import "MWMDropDown.h"
#import "MWMEditorAddAdditionalNameTableViewCell.h"
#import "MWMEditorAdditionalNamesHeader.h"
#import "MWMEditorAdditionalNamesTableViewController.h"
#import "MWMEditorAdditionalNameTableViewCell.h"
#import "MWMEditorCategoryCell.h"
#import "MWMEditorCommon.h"
#import "MWMEditorNameFooter.h"
#import "MWMEditorNotesFooter.h"
#import "MWMEditorSelectTableViewCell.h"
#import "MWMEditorSwitchTableViewCell.h"
#import "MWMEditorTextTableViewCell.h"
#import "MWMEditorViewController.h"
#import "MWMNoteCell.h"
#import "MWMObjectsCategorySelectorController.h"
#import "MWMOpeningHoursEditorViewController.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageOpeningHoursCell.h"
#import "MWMStreetEditorViewController.h"
#import "Statistics.h"
#import "UIViewController+Navigation.h"

#include "std/algorithm.hpp"

namespace
{
NSString * const kAdditionalNamesEditorSegue = @"Editor2AdditionalNamesEditorSegue";
NSString * const kOpeningHoursEditorSegue = @"Editor2OpeningHoursEditorSegue";
NSString * const kCuisineEditorSegue = @"Editor2CuisineEditorSegue";
NSString * const kStreetEditorSegue = @"Editor2StreetEditorSegue";
NSString * const kCategoryEditorSegue = @"Editor2CategoryEditorSegue";
CGFloat const kDefaultHeaderHeight = 28.;
CGFloat const kDefaultFooterHeight = 32.;

typedef NS_ENUM(NSUInteger, MWMEditorSection)
{
  MWMEditorSectionCategory,
  MWMEditorSectionName,
  MWMEditorSectionAdditionalNames,
  MWMEditorSectionAddress,
  MWMEditorSectionDetails,
  MWMEditorSectionNote,
  MWMEditorSectionButton
};

vector<MWMPlacePageCellType> const kSectionCategoryCellTypes{MWMPlacePageCellTypeCategory};
vector<MWMPlacePageCellType> const kSectionNameCellTypes{MWMPlacePageCellTypeName};
vector<MWMPlacePageCellType> const kSectionAddressCellTypes{
    MWMPlacePageCellTypeStreet, MWMPlacePageCellTypeBuilding, MWMPlacePageCellTypeZipCode};

vector<MWMPlacePageCellType> const kSectionNoteCellTypes{MWMPlacePageCellTypeNote};
vector<MWMPlacePageCellType> const kSectionButtonCellTypes{MWMPlacePageCellTypeReportButton};

MWMPlacePageCellTypeValueMap const kCellType2ReuseIdentifier{
    {MWMPlacePageCellTypeCategory, "MWMEditorCategoryCell"},
    {MWMPlacePageCellTypeName, "MWMEditorNameTableViewCell"},
    {MWMPlacePageCellTypeAdditionalName, "MWMEditorAdditionalNameTableViewCell"},
    {MWMPlacePageCellTypeAddAdditionalName, "MWMEditorAddAdditionalNameTableViewCell"},
    {MWMPlacePageCellTypeAddAdditionalNamePlaceholder, "MWMEditorAdditionalNamePlaceholderTableViewCell"},
    {MWMPlacePageCellTypeStreet, "MWMEditorSelectTableViewCell"},
    {MWMPlacePageCellTypeBuilding, "MWMEditorTextTableViewCell"},
    {MWMPlacePageCellTypeZipCode, "MWMEditorTextTableViewCell"},
    {MWMPlacePageCellTypeBuildingLevels, "MWMEditorTextTableViewCell"},
    {MWMPlacePageCellTypeOpenHours, "MWMPlacePageOpeningHoursCell"},
    {MWMPlacePageCellTypePhoneNumber, "MWMEditorTextTableViewCell"},
    {MWMPlacePageCellTypeWebsite, "MWMEditorTextTableViewCell"},
    {MWMPlacePageCellTypeEmail, "MWMEditorTextTableViewCell"},
    {MWMPlacePageCellTypeOperator, "MWMEditorTextTableViewCell"},
    {MWMPlacePageCellTypeCuisine, "MWMEditorSelectTableViewCell"},
    {MWMPlacePageCellTypeWiFi, "MWMEditorSwitchTableViewCell"},
    {MWMPlacePageCellTypeNote, "MWMNoteCell"},
    {MWMPlacePageCellTypeReportButton, "MWMButtonCell"}};

NSString * reuseIdentifier(MWMPlacePageCellType cellType)
{
  auto const it = kCellType2ReuseIdentifier.find(cellType);
  BOOL const haveCell = (it != kCellType2ReuseIdentifier.end());
  ASSERT(haveCell, ());
  return haveCell ? @(it->second.c_str()) : @"";
}

vector<osm::LocalizedName> getAdditionalLocalizedNames(osm::EditableMapObject const & emo)
{
  vector<osm::LocalizedName> result;
  emo.GetName().ForEach([&result](int8_t code, string const & name) -> bool
  {
    if (code != StringUtf8Multilang::kDefaultCode)
      result.push_back({code, StringUtf8Multilang::GetLangByCode(code),
        StringUtf8Multilang::GetLangNameByCode(code), name});
    return true;
  });
  return result;
}

void cleanupAdditionalLanguages(vector<osm::LocalizedName> const & names, vector<NSInteger> & newAdditionalLanguages)
{
  newAdditionalLanguages.erase(remove_if(newAdditionalLanguages.begin(),
                                         newAdditionalLanguages.end(),
                                         [&names](NSInteger x)
  {
    auto it = find_if(names.begin(), names.end(), [x](osm::LocalizedName const & name)
    {
      return name.m_code == x;
    });
    return it != names.end();
  }),
                               newAdditionalLanguages.end());
}

vector<MWMPlacePageCellType> cellsForAdditionalNames(
    vector<osm::LocalizedName> const & names, vector<NSInteger> const & newAdditionalLanguages,
    BOOL showAdditionalNames)
{
  if (names.empty() && newAdditionalLanguages.empty())
    return vector<MWMPlacePageCellType>();
  vector<MWMPlacePageCellType> res;
  if (showAdditionalNames)
  {
    res.insert(res.begin(), names.size() + newAdditionalLanguages.size(),
               MWMPlacePageCellTypeAdditionalName);
  }
  else
  {
    res.push_back(MWMPlacePageCellTypeAdditionalName);
    res.push_back(MWMPlacePageCellTypeAddAdditionalNamePlaceholder);
  }
  res.push_back(MWMPlacePageCellTypeAddAdditionalName);
  return res;
}

vector<MWMPlacePageCellType> cellsForProperties(vector<osm::Props> const & props)
{
  using namespace osm;
  vector<MWMPlacePageCellType> res;
  for (auto const p : props)
  {
    switch (p)
    {
    case Props::OpeningHours:
      res.push_back(MWMPlacePageCellTypeOpenHours);
      break;
    case Props::Phone:
      res.push_back(MWMPlacePageCellTypePhoneNumber);
      break;
    case Props::Website:
      res.push_back(MWMPlacePageCellTypeWebsite);
      break;
    case Props::Email:
      res.push_back(MWMPlacePageCellTypeEmail);
      break;
    case Props::Cuisine:
      res.push_back(MWMPlacePageCellTypeCuisine);
      break;
    case Props::Operator:
      res.push_back(MWMPlacePageCellTypeOperator);
      break;
    case Props::Internet:
      res.push_back(MWMPlacePageCellTypeWiFi);
      break;
    case Props::Wikipedia:
    case Props::Fax:
    case Props::Stars:
    case Props::Elevation:
    case Props::Flats:
    case Props::BuildingLevels:
      break;
    }
  }
  return res;
}

void registerCellsForTableView(vector<MWMPlacePageCellType> const & cells, UITableView * tv)
{
  for (auto const c : cells)
  {
    NSString * identifier = reuseIdentifier(c);
    if (UINib * nib = [UINib nibWithNibName:identifier bundle:nil])
      [tv registerNib:nib forCellReuseIdentifier:identifier];
    else
      ASSERT(false, ("Incorrect cell"));
  }
}
} // namespace

@interface MWMEditorViewController ()<
    UITableViewDelegate, UITableViewDataSource, UITextFieldDelegate, MWMOpeningHoursEditorProtocol,
    MWMPlacePageOpeningHoursCellProtocol, MWMEditorCellProtocol, MWMCuisineEditorProtocol,
    MWMStreetEditorProtocol, MWMObjectsCategorySelectorDelegate, MWMNoteCelLDelegate,
    MWMEditorAdditionalName, MWMButtonCellDelegate, MWMEditorAdditionalNamesProtocol>

@property (nonatomic) NSMutableDictionary<NSString *, UITableViewCell *> * offscreenCells;
@property (nonatomic) NSMutableArray<NSIndexPath *> * invalidCells;
@property (nonatomic) MWMEditorAdditionalNamesHeader * additionalNamesHeader;
@property (nonatomic) MWMEditorNotesFooter * notesFooter;
@property (nonatomic) MWMEditorNameFooter * nameFooter;
@property (copy, nonatomic) NSString * note;
@property (nonatomic) osm::Editor::FeatureStatus featureStatus;
@property (nonatomic) BOOL isFeatureUploaded;

@property (nonatomic) BOOL showAdditionalNames;

@end

@implementation MWMEditorViewController
{
  vector<MWMEditorSection> m_sections;
  map<MWMEditorSection, vector<MWMPlacePageCellType>> m_cells;
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
  self.title = L(self.isCreating ? @"editor_add_place_title" : @"editor_edit_place_title").capitalizedString;
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
    MWMEditorTextTableViewCell * cell = [self.tableView cellForRowAtIndexPath:self.invalidCells.firstObject];
    [cell.textField becomeFirstResponder];
    return;
  }

  auto & f = GetFramework();
  auto const & featureID = m_mapObject.GetID();
  NSDictionary<NSString *, NSString *> * info = @{kStatEditorMWMName : @(featureID.GetMwmName().c_str()),
                          kStatEditorMWMVersion : @(featureID.GetMwmVersion())};
  BOOL const haveNote = self.note.length;

  if (haveNote)
  {
    auto const latLon = m_mapObject.GetLatLon();
    NSMutableDictionary * noteInfo = [info mutableCopy];
    noteInfo[kStatProblem] = self.note;
    noteInfo[kStatLat] = @(latLon.lat);
    noteInfo[kStatLon] = @(latLon.lon);
    [Statistics logEvent:kStatEditorProblemReport withParameters:noteInfo];
    f.CreateNote(latLon, featureID, osm::Editor::NoteProblemType::General, self.note.UTF8String);
  }

  switch (f.SaveEditedMapObject(m_mapObject))
  {
    case osm::Editor::NothingWasChanged:
      [self.navigationController popToRootViewControllerAnimated:YES];
      if (haveNote)
        [self showDropDown];
      break;
    case osm::Editor::SavedSuccessfully:
      [Statistics logEvent:(self.isCreating ? kStatEditorAddSuccess : kStatEditorEditSuccess) withParameters:info];
      osm_auth_ios::AuthorizationSetNeedCheck(YES);
      f.UpdatePlacePageInfoForCurrentSelection();
      [self.navigationController popToRootViewControllerAnimated:YES];
      break;
    case osm::Editor::NoFreeSpaceError:
      [Statistics logEvent:(self.isCreating ? kStatEditorAddError : kStatEditorEditError) withParameters:info];
      [self.alertController presentNotEnoughSpaceAlert];
      break;
  }
}

- (void)showDropDown
{
  UIViewController * parent = static_cast<UIViewController *>([MapsAppDelegate theApp].mapViewController);
  MWMDropDown * dd = [[MWMDropDown alloc] initWithSuperview:parent.view];
  [dd showWithMessage:L(@"editor_edits_sent_message")];
}

#pragma mark - Headers

- (MWMEditorAdditionalNamesHeader *)additionalNamesHeader
{
  if (!_additionalNamesHeader)
  {
    __weak auto weakSelf = self;
    _additionalNamesHeader = [MWMEditorAdditionalNamesHeader header:^
    {
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

- (MWMEditorNameFooter *)nameFooter
{
  if (!_nameFooter)
    _nameFooter = [MWMEditorNameFooter footer];
  return _nameFooter;
}

#pragma mark - Properties

- (void)setShowAdditionalNames:(BOOL)showAdditionalNames
{
  _showAdditionalNames = showAdditionalNames;
  [self.additionalNamesHeader setShowAdditionalNames:showAdditionalNames];
  [self configTable];
  auto const additionalNamesSectionIt = find(m_sections.begin(), m_sections.end(), MWMEditorSectionAdditionalNames);
  if (additionalNamesSectionIt == m_sections.end())
  {
    [self.tableView reloadData];
  }
  else
  {
    auto const sectionIndex = distance(m_sections.begin(), additionalNamesSectionIt);
    [self.tableView reloadSections:[[NSIndexSet alloc] initWithIndex:sectionIndex] withRowAnimation:UITableViewRowAnimationAutomatic];
  }
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

- (void)configTable
{
  self.offscreenCells = [NSMutableDictionary dictionary];
  self.invalidCells = [NSMutableArray array];
  m_sections.clear();
  m_cells.clear();

  m_sections.push_back(MWMEditorSectionCategory);
  m_cells[MWMEditorSectionCategory] = kSectionCategoryCellTypes;
  registerCellsForTableView(kSectionCategoryCellTypes, self.tableView);

  if (m_mapObject.IsNameEditable())
  {
    m_sections.push_back(MWMEditorSectionName);
    m_cells[MWMEditorSectionName] = kSectionNameCellTypes;
    registerCellsForTableView(kSectionNameCellTypes, self.tableView);

    vector<osm::LocalizedName> localizedNames = getAdditionalLocalizedNames(m_mapObject);
    cleanupAdditionalLanguages(localizedNames, m_newAdditionalLanguages);
    auto const cells = cellsForAdditionalNames(localizedNames, m_newAdditionalLanguages, self.showAdditionalNames);
    if (!cells.empty())
    {
      m_sections.push_back(MWMEditorSectionAdditionalNames);
      m_cells[MWMEditorSectionAdditionalNames] = cells;
      registerCellsForTableView(cells, self.tableView);
    }
  }
  if (m_mapObject.IsAddressEditable())
  {
    m_sections.push_back(MWMEditorSectionAddress);
    m_cells[MWMEditorSectionAddress] = kSectionAddressCellTypes;
    if (m_mapObject.IsBuilding())
      m_cells[MWMEditorSectionAddress].push_back(MWMPlacePageCellTypeBuildingLevels);

    registerCellsForTableView(kSectionAddressCellTypes, self.tableView);
  }
  if (!m_mapObject.GetEditableProperties().empty())
  {
    auto const cells = cellsForProperties(m_mapObject.GetEditableProperties());
    if (!cells.empty())
    {
      m_sections.push_back(MWMEditorSectionDetails);
      m_cells[MWMEditorSectionDetails] = cells;
      registerCellsForTableView(cells, self.tableView);
    }
  }
  m_sections.push_back(MWMEditorSectionNote);
  m_cells[MWMEditorSectionNote] = kSectionNoteCellTypes;
  registerCellsForTableView(kSectionNoteCellTypes, self.tableView);

  if (self.isCreating)
    return;
  m_sections.push_back(MWMEditorSectionButton);
  m_cells[MWMEditorSectionButton] = kSectionButtonCellTypes;
  registerCellsForTableView(kSectionButtonCellTypes, self.tableView);
}

- (MWMPlacePageCellType)cellTypeForIndexPath:(NSIndexPath *)indexPath
{
  return m_cells[m_sections[indexPath.section]][indexPath.row];
}

- (NSString *)cellIdentifierForIndexPath:(NSIndexPath *)indexPath
{
  return reuseIdentifier([self cellTypeForIndexPath:indexPath]);
}

#pragma mark - Fill cells with data

- (void)fillCell:(UITableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  BOOL const isValid = ![self.invalidCells containsObject:indexPath];
  switch ([self cellTypeForIndexPath:indexPath])
  {
    case MWMPlacePageCellTypeCategory:
    {
      MWMEditorCategoryCell * cCell = static_cast<MWMEditorCategoryCell *>(cell);
      [cCell configureWithDelegate:self detailTitle:@(m_mapObject.GetLocalizedType().c_str()) isCreating:self.isCreating];
      break;
    }
    case MWMPlacePageCellTypePhoneNumber:
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
    case MWMPlacePageCellTypeWebsite:
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
    case MWMPlacePageCellTypeEmail:
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
    case MWMPlacePageCellTypeOperator:
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
    case MWMPlacePageCellTypeOpenHours:
    {
      MWMPlacePageOpeningHoursCell * tCell = static_cast<MWMPlacePageOpeningHoursCell *>(cell);
      NSString * text = @(m_mapObject.GetOpeningHours().c_str());
      [tCell configWithDelegate:self info:(text.length ? text : L(@"add_opening_hours"))];
      break;
    }
    case MWMPlacePageCellTypeWiFi:
    {
      MWMEditorSwitchTableViewCell * tCell = static_cast<MWMEditorSwitchTableViewCell *>(cell);
      // TODO(Vlad, IgorTomko): Support all other possible Internet statuses.
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_wifi"]
                           text:L(@"wifi")
                             on:m_mapObject.GetInternet() == osm::Internet::Wlan];
      break;
    }
    case MWMPlacePageCellTypeName:
    {
      MWMEditorTextTableViewCell * tCell = static_cast<MWMEditorTextTableViewCell *>(cell);
      [tCell configWithDelegate:self
                           icon:nil
                           text:@(m_mapObject.GetDefaultName().c_str())
                    placeholder:L(@"name")
                   keyboardType:UIKeyboardTypeDefault
                 capitalization:UITextAutocapitalizationTypeSentences];
      break;
    }
    case MWMPlacePageCellTypeAdditionalName:
    {
      MWMEditorAdditionalNameTableViewCell * tCell = static_cast<MWMEditorAdditionalNameTableViewCell *>(cell);

      vector<osm::LocalizedName> const localizedNames = getAdditionalLocalizedNames(m_mapObject);

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
        [tCell configWithDelegate:self
                         langCode:langCode
                         langName:@(StringUtf8Multilang::GetLangNameByCode(langCode))
                             name:@""
                     keyboardType:UIKeyboardTypeDefault];
      }
      break;
    }
    case MWMPlacePageCellTypeAddAdditionalName:
    {
      MWMEditorAddAdditionalNameTableViewCell * tCell = static_cast<MWMEditorAddAdditionalNameTableViewCell *>(cell);
      [tCell configWithDelegate:self];
      break;
    }
    case MWMPlacePageCellTypeAddAdditionalNamePlaceholder:
      break;
    case MWMPlacePageCellTypeStreet:
    {
      MWMEditorSelectTableViewCell * tCell = static_cast<MWMEditorSelectTableViewCell *>(cell);
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_adress"]
                           text:@(m_mapObject.GetStreet().m_defaultName.c_str())
                    placeholder:L(@"add_street")];
      break;
    }
    case MWMPlacePageCellTypeBuilding:
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
    case MWMPlacePageCellTypeZipCode:
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
    case MWMPlacePageCellTypeBuildingLevels:
    {
      NSString * placeholder = [NSString stringWithFormat:L(@"editor_storey_number"),
                                                          osm::EditableMapObject::kMaximumLevelsEditableByUsers];
      NSString * errorMessage = [NSString stringWithFormat:L(@"error_enter_correct_storey_number"),
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
    case MWMPlacePageCellTypeCuisine:
    {
      MWMEditorSelectTableViewCell * tCell = static_cast<MWMEditorSelectTableViewCell *>(cell);
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_cuisine"]
                           text:@(m_mapObject.FormatCuisines().c_str())
                    placeholder:L(@"select_cuisine")];
      break;
    }
    case MWMPlacePageCellTypeNote:
    {
      MWMNoteCell * tCell = static_cast<MWMNoteCell *>(cell);
      [tCell configWithDelegate:self noteText:self.note
                    placeholder:L(@"editor_detailed_description_hint")];
      break;
    }
    case MWMPlacePageCellTypeReportButton:
    {
      MWMButtonCell * tCell = static_cast<MWMButtonCell *>(cell);

      auto title = ^ NSString * (osm::Editor::FeatureStatus s, BOOL isUploaded)
      {
        if (isUploaded)
          return L(@"editor_place_doesnt_exist");
        switch (s)
        {
        case osm::Editor::FeatureStatus::Untouched:
          return L(@"editor_place_doesnt_exist");
        case osm::Editor::FeatureStatus::Deleted:
        case osm::Editor::FeatureStatus::Obsolete:  // TODO(Vlad): Either make a valid button or disable it.
          NSAssert(false, @"Incorrect feature status!");
          return L(@"editor_place_doesnt_exist");
        case osm::Editor::FeatureStatus::Modified:
          return L(@"editor_reset_edits_button");
        case osm::Editor::FeatureStatus::Created:
          return L(@"editor_remove_place_button");
        }
      };

      [tCell configureWithDelegate:self title:title(self.featureStatus, self.isFeatureUploaded)];
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
  UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:reuseIdentifier];
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

- (CGFloat)tableView:(UITableView * _Nonnull)tableView heightForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSString * reuseIdentifier = [self cellIdentifierForIndexPath:indexPath];

  UITableViewCell * cell = [self offscreenCellForIdentifier:reuseIdentifier];
  [self fillCell:cell atIndexPath:indexPath];
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case MWMPlacePageCellTypeOpenHours:
      return ((MWMPlacePageOpeningHoursCell *)cell).cellHeight;
    case MWMPlacePageCellTypeCategory:
    case MWMPlacePageCellTypeReportButton:
      return self.tableView.rowHeight;
    case MWMPlacePageCellTypeNote:
      return static_cast<MWMNoteCell *>(cell).cellHeight;
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

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
  switch (m_sections[section])
  {
  case MWMEditorSectionName:
  case MWMEditorSectionAdditionalNames:
  case MWMEditorSectionCategory:
  case MWMEditorSectionButton:
    return nil;
  case MWMEditorSectionNote:
    return L(@"editor_other_info");
  case MWMEditorSectionAddress:
    return L(@"address");
  case MWMEditorSectionDetails:
    return L(@"details");
  }
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section
{
  switch (m_sections[section])
  {
    case MWMEditorSectionAdditionalNames:
      return self.additionalNamesHeader;
    case MWMEditorSectionName:
    case MWMEditorSectionCategory:
    case MWMEditorSectionButton:
    case MWMEditorSectionNote:
    case MWMEditorSectionAddress:
    case MWMEditorSectionDetails:
      return nil;
  }
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section
{
  switch (m_sections[section])
  {
  case MWMEditorSectionAddress:
  case MWMEditorSectionDetails:
  case MWMEditorSectionCategory:
  case MWMEditorSectionAdditionalNames:
  case MWMEditorSectionButton:
    return nil;
  case MWMEditorSectionName:
    return self.nameFooter;
  case MWMEditorSectionNote:
    return self.notesFooter;
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
  case MWMEditorSectionDetails:
  case MWMEditorSectionCategory:
  case MWMEditorSectionAdditionalNames:
    return 1.0;
  case MWMEditorSectionNote:
    return self.notesFooter.height;
  case MWMEditorSectionName:
    return self.nameFooter.height;
  case MWMEditorSectionButton:
    return kDefaultFooterHeight;
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

- (void)setOpeningHoursCellExpanded:(BOOL)openingHoursCellExpanded
{
  [self performSegueWithIdentifier:kOpeningHoursEditorSegue sender:nil];
}

- (void)markCellAsInvalid:(NSIndexPath *)indexPath
{
  if (![self.invalidCells containsObject:indexPath])
    [self.invalidCells addObject:indexPath];

  [self.tableView reloadRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
}

#pragma mark - MWMNoteCellDelegate

- (void)cellShouldChangeSize:(MWMNoteCell *)cell text:(NSString *)text
{
  self.offscreenCells[reuseIdentifier(MWMPlacePageCellTypeNote)] = cell;
  self.note = text;
  [self.tableView beginUpdates];
  [self.tableView endUpdates];
  [self.tableView scrollToRowAtIndexPath:[self.tableView indexPathForCell:cell]
                        atScrollPosition:UITableViewScrollPositionBottom
                                animated:YES];
}

- (void)cell:(MWMNoteCell *)cell didFinishEditingWithText:(NSString *)text
{
  self.note = text;
}

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
  auto additionalNamesSectionIt = find(m_sections.begin(), m_sections.end(), MWMEditorSectionAdditionalNames);
  assert(additionalNamesSectionIt != m_sections.end());
  auto const section = distance(m_sections.begin(), additionalNamesSectionIt);
  NSInteger const row = [self tableView:self.tableView numberOfRowsInSection:section];
  assert(row > 0);
  NSIndexPath * indexPath = [NSIndexPath indexPathForRow:row - 1 inSection:section];
  [self.tableView scrollToRowAtIndexPath:indexPath atScrollPosition:UITableViewScrollPositionMiddle animated:NO];
}

#pragma mark - MWMEditorCellProtocol

- (void)tryToChangeInvalidStateForCell:(MWMEditorTextTableViewCell *)cell
{
  [self.tableView beginUpdates];

  NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
  [self.invalidCells removeObject:indexPath];

  [self.tableView endUpdates];
}

- (void)cell:(MWMTableViewCell *)cell changedText:(NSString *)changeText
{
  NSAssert(changeText != nil, @"String can't be nil!");
  NSIndexPath * indexPath = [self.tableView indexPathForRowAtPoint:cell.center];
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  string const val = changeText.UTF8String;
  switch (cellType)
  {
    case MWMPlacePageCellTypeName: m_mapObject.SetName(val, StringUtf8Multilang::kDefaultCode); break;
    case MWMPlacePageCellTypePhoneNumber:
      m_mapObject.SetPhone(val);
      if (!osm::EditableMapObject::ValidatePhone(val))
        [self markCellAsInvalid:indexPath];
      break;
    case MWMPlacePageCellTypeWebsite:
      m_mapObject.SetWebsite(val);
      if (!osm::EditableMapObject::ValidateWebsite(val))
        [self markCellAsInvalid:indexPath];
      break;
    case MWMPlacePageCellTypeEmail:
      m_mapObject.SetEmail(val);
      if (!osm::EditableMapObject::ValidateEmail(val))
        [self markCellAsInvalid:indexPath];
      break;
    case MWMPlacePageCellTypeOperator: m_mapObject.SetOperator(val); break;
    case MWMPlacePageCellTypeBuilding:
      m_mapObject.SetHouseNumber(val);
      if (!osm::EditableMapObject::ValidateHouseNumber(val))
        [self markCellAsInvalid:indexPath];
      break;
    case MWMPlacePageCellTypeZipCode:
      m_mapObject.SetPostcode(val);
      if (!osm::EditableMapObject::ValidatePostCode(val))
        [self markCellAsInvalid:indexPath];
      break;
    case MWMPlacePageCellTypeBuildingLevels:
      m_mapObject.SetBuildingLevels(val);
      if (!osm::EditableMapObject::ValidateBuildingLevels(val))
        [self markCellAsInvalid:indexPath];
      break;
    case MWMPlacePageCellTypeAdditionalName:
      {
        MWMEditorAdditionalNameTableViewCell * tCell = static_cast<MWMEditorAdditionalNameTableViewCell *>(cell);
        m_mapObject.SetName(val, tCell.code);
        break;
      }
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

#pragma mark - MWMEditorCellProtocol && MWMButtonCellDelegate

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
  case MWMPlacePageCellTypeCategory:
    [self performSegueWithIdentifier:kCategoryEditorSegue sender:nil];
    break;
  case MWMPlacePageCellTypeReportButton:
    [self tapOnButtonCell:cell];
    break;
  default:
    NSAssert(false, @"Invalid field for cellSelect");
    break;
  }
}

- (void)tapOnButtonCell:(UITableViewCell *)cell
{
  auto const & fid = m_mapObject.GetID();
  auto const latLon = m_mapObject.GetLatLon();
  self.isFeatureUploaded = osm::Editor::Instance().IsFeatureUploaded(fid.m_mwmId, fid.m_index);
  [self.tableView reloadRowsAtIndexPaths:@[[self.tableView indexPathForCell:cell]]
                        withRowAnimation:UITableViewRowAnimationFade];

  auto placeDoesntExistAction = ^
  {
    [self.alertController presentPlaceDoesntExistAlertWithBlock:^(NSString * additionalMessage)
    {
       string const additional = additionalMessage.length ? additionalMessage.UTF8String : "";
       [Statistics logEvent:kStatEditorProblemReport withParameters:@{
                                                            kStatEditorMWMName : @(fid.GetMwmName().c_str()),
                                                            kStatEditorMWMVersion : @(fid.GetMwmVersion()),
                                                            kStatProblem : @(osm::Editor::kPlaceDoesNotExistMessage),
                                                            kStatLat : @(latLon.lat), kStatLon : @(latLon.lon)}];
       GetFramework().CreateNote(latLon, fid, osm::Editor::NoteProblemType::PlaceDoesNotExist, additional);
       [self backTap];
       [self showDropDown];
     }];
  };

  auto revertAction = ^(BOOL isCreated)
  {
    [Statistics logEvent:isCreated ? kStatEditorAddCancel : kStatEditorEditCancel withParameters:@{
                                                                   kStatEditorMWMName : @(fid.GetMwmName().c_str()),
                                                                   kStatEditorMWMVersion : @(fid.GetMwmVersion()),
                                                                   kStatLat : @(latLon.lat), kStatLon : @(latLon.lon)}];
    auto & f = GetFramework();
    if (!f.RollBackChanges(fid))
      NSAssert(false, @"We shouldn't call this if we can't roll back!");

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
    case osm::Editor::FeatureStatus::Untouched:
      placeDoesntExistAction();
      break;
    case osm::Editor::FeatureStatus::Modified:
    {
      [self.alertController presentResetChangesAlertWithBlock:^
      {
         revertAction(NO);
      }];
      break;
    }
    case osm::Editor::FeatureStatus::Created:
    {
      [self.alertController presentDeleteFeatureAlertWithBlock:^
      {
         revertAction(YES);
      }];
      break;
    }
    case osm::Editor::FeatureStatus::Deleted:
      break;
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

- (vector<string>)getSelectedCuisines
{
  return m_mapObject.GetCuisines();
}

- (void)setSelectedCuisines:(vector<string> const &)cuisines
{
  m_mapObject.SetCuisines(cuisines);
}

#pragma mark - MWMStreetEditorProtocol

- (void)setNearbyStreet:(osm::LocalizedStreet const &)street
{
  m_mapObject.SetStreet(street);
}

- (osm::LocalizedStreet const &)currentStreet
{
  return m_mapObject.GetStreet();
}

- (vector<osm::LocalizedStreet> const &)nearbyStreets
{
  return m_mapObject.GetNearbyStreets();
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
  else if ([segue.identifier isEqualToString:kCategoryEditorSegue])
  {
    NSAssert(self.isCreating, @"Invalid state! We'll be able to change feature category only if we are creating feature!");
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

@end
