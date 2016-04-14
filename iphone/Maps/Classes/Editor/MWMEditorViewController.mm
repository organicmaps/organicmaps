#import "MWMAlertViewController.h"
#import "MWMAuthorizationCommon.h"
#import "MWMCuisineEditorViewController.h"
#import "MWMEditorCategoryCell.h"
#import "MWMEditorCommon.h"
#import "MWMEditorSelectTableViewCell.h"
#import "MWMEditorSwitchTableViewCell.h"
#import "MWMEditorTextTableViewCell.h"
#import "MWMEditorViewController.h"
#import "MWMObjectsCategorySelectorController.h"
#import "MWMOpeningHoursEditorViewController.h"
#import "MWMPlacePageEntity.h"
#import "MWMPlacePageOpeningHoursCell.h"
#import "MWMStreetEditorViewController.h"
#import "Statistics.h"

#import "UIViewController+Navigation.h"

#include "indexer/editable_map_object.hpp"
#include "std/algorithm.hpp"

namespace
{
NSString * const kOpeningHoursEditorSegue = @"Editor2OpeningHoursEditorSegue";
NSString * const kCuisineEditorSegue = @"Editor2CuisineEditorSegue";
NSString * const kStreetEditorSegue = @"Editor2StreetEditorSegue";
NSString * const kCategoryEditorSegue = @"Editor2CategoryEditorSegue";

typedef NS_ENUM(NSUInteger, MWMEditorSection)
{
  MWMEditorSectionCategory,
  MWMEditorSectionName,
  MWMEditorSectionAddress,
  MWMEditorSectionDetails
};

vector<MWMPlacePageCellType> const kSectionCategoryCellTypes{MWMPlacePageCellTypeCategory};

vector<MWMPlacePageCellType> const kSectionNameCellTypes{MWMPlacePageCellTypeName};

vector<MWMPlacePageCellType> const kSectionAddressCellTypes{
    MWMPlacePageCellTypeStreet, MWMPlacePageCellTypeBuilding, MWMPlacePageCellTypeZipCode};

MWMPlacePageCellTypeValueMap const kCellType2ReuseIdentifier{
    {MWMPlacePageCellTypeCategory, "MWMEditorCategoryCell"},
    {MWMPlacePageCellTypeName, "MWMEditorNameTableViewCell"},
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
    {MWMPlacePageCellTypeWiFi, "MWMEditorSwitchTableViewCell"}};

NSString * reuseIdentifier(MWMPlacePageCellType cellType)
{
  auto const it = kCellType2ReuseIdentifier.find(cellType);
  BOOL const haveCell = (it != kCellType2ReuseIdentifier.end());
  ASSERT(haveCell, ());
  return haveCell ? @(it->second.c_str()) : @"";
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

@interface MWMEditorViewController() <UITableViewDelegate, UITableViewDataSource,
                                      UITextFieldDelegate, MWMOpeningHoursEditorProtocol,
                                      MWMPlacePageOpeningHoursCellProtocol,
                                      MWMEditorCellProtocol, MWMCuisineEditorProtocol,
                                      MWMStreetEditorProtocol, MWMObjectsCategorySelectorDelegate>

@property (nonatomic) NSMutableDictionary<NSString *, UITableViewCell *> * offscreenCells;
@property (nonatomic) NSMutableArray<NSIndexPath *> * invalidCells;

@end

@implementation MWMEditorViewController
{
  vector<MWMEditorSection> m_sections;
  map<MWMEditorSection, vector<MWMPlacePageCellType>> m_cells;
  osm::EditableMapObject m_mapObject;
}

- (void)viewDidLoad
{
  [Statistics logEvent:kStatEventName(kStatEdit, kStatOpen)];
  [super viewDidLoad];
  [self configTable];
  [self configNavBar];
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
  NSDictionary * info = @{kStatEditorMWMName : @(featureID.GetMwmName().c_str()),
                          kStatEditorMWMVersion : @(featureID.GetMwmVersion())};

  switch (f.SaveEditedMapObject(m_mapObject))
  {
    case osm::Editor::NothingWasChanged: 
      [self.navigationController popToRootViewControllerAnimated:YES];
      break;
    case osm::Editor::SavedSuccessfully:
      [Statistics logEvent:(self.isCreating ? kStatEditorAddSuccess : kStatEditorEditSuccess) withParameters:info];
      osm_auth_ios::AuthorizationSetNeedCheck(YES);
      f.UpdatePlacePageInfoForCurrentSelection();
      [self.navigationController popToRootViewControllerAnimated:YES];
      break;
    case osm::Editor::NoFreeSpaceError:
      [Statistics logEvent:(self.isCreating ? kStatEditorAddError : kStatEditorEditError) withParameters:info];
      [self.alertController presentDownloaderNotEnoughSpaceAlert];
      break;
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

  m_sections.push_back(MWMEditorSectionCategory);
  m_cells[MWMEditorSectionCategory] = kSectionCategoryCellTypes;
  registerCellsForTableView(kSectionCategoryCellTypes, self.tableView);

  if (m_mapObject.IsNameEditable())
  {
    m_sections.push_back(MWMEditorSectionName);
    m_cells[MWMEditorSectionName] = kSectionNameCellTypes;
    registerCellsForTableView(kSectionNameCellTypes, self.tableView);
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
      MWMEditorTextTableViewCell * tCell = (MWMEditorTextTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_phone_number"]
                           text:@(m_mapObject.GetPhone().c_str())
                    placeholder:L(@"phone")
                   keyboardType:UIKeyboardTypeNamePhonePad];
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
    case MWMPlacePageCellTypeOperator:
    {
      MWMEditorTextTableViewCell * tCell = static_cast<MWMEditorTextTableViewCell *>(cell);
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_operator"]
                           text:@(m_mapObject.GetOperator().c_str())
                    placeholder:L(@"editor_operator")
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
                           text:@(m_mapObject.GetStreet().m_defaultName.c_str())
                    placeholder:L(@"add_street")];
      break;
    }
    case MWMPlacePageCellTypeBuilding:
    {
      MWMEditorTextTableViewCell * tCell = (MWMEditorTextTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:nil
                           text:@(m_mapObject.GetHouseNumber().c_str())
                    placeholder:L(@"house_number")
                   errorMessage:L(@"error_enter_correct_house_number")
                        isValid:isValid
                   keyboardType:UIKeyboardTypeDefault];
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
                   keyboardType:UIKeyboardTypeDefault];
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
                   keyboardType:UIKeyboardTypeNumberPad];
      break;
    }
    case MWMPlacePageCellTypeCuisine:
    {
      MWMEditorSelectTableViewCell * tCell = (MWMEditorSelectTableViewCell *)cell;
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
  // TODO(Vlad, IGrechuhin): It's bad idea to fill cells here.
  // heightForRowAtIndexPath is called way too often for the table.
  [self fillCell:cell atIndexPath:indexPath];
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case MWMPlacePageCellTypeOpenHours:
      return ((MWMPlacePageOpeningHoursCell *)cell).cellHeight;
    case MWMPlacePageCellTypeCategory:
      return self.tableView.rowHeight;
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
  case MWMEditorSectionCategory:
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
  case MWMEditorSectionCategory:
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

- (void)markCellAsInvalid:(NSIndexPath *)indexPath
{
  if (![self.invalidCells containsObject:indexPath])
    [self.invalidCells addObject:indexPath];

  [self.tableView reloadRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
}

#pragma mark - MWMEditorCellProtocol

- (void)tryToChangeInvalidStateForCell:(MWMEditorTextTableViewCell *)cell
{
  [self.tableView beginUpdates];
  
  NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
  [self.invalidCells removeObject:indexPath];

  [self.tableView endUpdates];
}

- (void)cell:(MWMEditorTextTableViewCell *)cell changedText:(NSString *)changeText
{
  NSAssert(changeText != nil, @"String can't be nil!");
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
    case MWMPlacePageCellTypeOperator: m_mapObject.SetOperator(val); break;
    case MWMPlacePageCellTypeBuilding:
      m_mapObject.SetHouseNumber(val);
      if (!osm::EditableMapObject::ValidateHouseNumber(val))
        [self markCellAsInvalid:indexPath];
      break;
    case MWMPlacePageCellTypeZipCode:
      m_mapObject.SetPostcode(val);
      // TODO: Validate postcode.
      break;
    case MWMPlacePageCellTypeBuildingLevels:
      m_mapObject.SetBuildingLevels(val);
      if (!osm::EditableMapObject::ValidateBuildingLevels(val))
        [self markCellAsInvalid:indexPath];
      break;
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
    case MWMPlacePageCellTypeCategory:
      [self performSegueWithIdentifier:kCategoryEditorSegue sender:nil];
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
    MWMObjectsCategorySelectorController * dest = segue.destinationViewController;
    dest.delegate = self;
    [dest setSelectedCategory:m_mapObject.GetLocalizedType()];
  }
}

@end
