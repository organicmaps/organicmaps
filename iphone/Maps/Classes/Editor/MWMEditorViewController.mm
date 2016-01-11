#import "MWMCuisineEditorViewController.h"
#import "MWMEditorCommon.h"
#import "MWMEditorSelectTableViewCell.h"
#import "MWMEditorSwitchTableViewCell.h"
#import "MWMEditorTableViewHeader.h"
#import "MWMEditorTextTableViewCell.h"
#import "MWMEditorViewController.h"
#import "MWMOpeningHoursEditorViewController.h"
#import "MWMPlacePageOpeningHoursCell.h"
#import "MWMStreetEditorViewController.h"

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
    {MWMPlacePageCellTypeStreet, MWMPlacePageCellTypeBuilding, MWMPlacePageCellTypeSpacer}};

vector<MWMPlacePageCellType> const gSectionDetailsCellTypes{
    {MWMPlacePageCellTypeOpenHours, MWMPlacePageCellTypePhoneNumber, MWMPlacePageCellTypeWebsite,
     MWMPlacePageCellTypeEmail, MWMPlacePageCellTypeCuisine, MWMPlacePageCellTypeWiFi,
     MWMPlacePageCellTypeSpacer}};

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
    {MWMPlacePageCellTypeWiFi, "MWMEditorSwitchTableViewCell"},
    {MWMPlacePageCellTypeSpacer, "MWMEditorSpacerCell"}};

NSString * reuseIdentifier(MWMPlacePageCellType cellType)
{
  auto const it = gCellType2ReuseIdentifier.find(cellType);
  BOOL const haveCell = (it != gCellType2ReuseIdentifier.end());
  ASSERT(haveCell, ());
  return haveCell ? @(it->second.c_str()) : @"";
}
} // namespace

@interface MWMEditorViewController ()<UITableViewDelegate, UITableViewDataSource,
                                      UITextFieldDelegate, MWMPlacePageOpeningHoursCellProtocol,
                                      MWMEditorCellProtocol, MWMCuisineEditorProtocol,
                                      MWMStreetEditorProtocol>

@property (weak, nonatomic) IBOutlet UITableView * tableView;

@property (nonatomic) NSMutableDictionary<NSString *, UITableViewCell *> * offscreenCells;

@property (nonatomic) BOOL needsReload;

@end

@implementation MWMEditorViewController
{
  vector<MWMEditorSection> m_sections;
  map<MWMEditorSection, vector<MWMPlacePageCellType>> m_cells;
  MWMPlacePageCellTypeValueMap m_edited_cells;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configTable];
  [self configNavBar];
}

- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
  if (self.needsReload)
  {
    [self.tableView reloadData];
    self.needsReload = NO;
  }
}

#pragma mark - Configuration

- (void)configNavBar
{
  self.title = L(@"edit_poi");
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
  [self dismissViewControllerAnimated:YES completion:nil];
}

- (void)onSave
{
  [self.entity saveEditedCells:m_edited_cells];
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

#pragma mark - Data source

- (NSString *)getCellValue:(MWMPlacePageCellType)cellType
{
  auto const it = m_edited_cells.find(cellType);
  if (it != m_edited_cells.end())
    return @(it->second.c_str());
  return [self.entity getCellValue:cellType];
}

- (void)setCell:(MWMPlacePageCellType)cellType value:(NSString *)value
{
  if ([value isEqualToString:[self getCellValue:cellType]])
    return;
  self.needsReload = YES;
  m_edited_cells[cellType] = value.UTF8String;
}

#pragma mark - Table

- (void)configTable
{
  NSAssert(self.entity, @"Entity must be set");
  self.offscreenCells = [NSMutableDictionary dictionary];
  m_sections.clear();
  m_cells.clear();
  m_edited_cells.clear();
  for (auto cellsSection : gCellTypesSectionMap)
  {
    for (auto cellType : cellsSection.first)
    {
      if (![self.entity isCellEditable:cellType])
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

- (BOOL)isLastCellInSection:(NSIndexPath *)indexPath
{
  NSUInteger const row = indexPath.row;
  NSUInteger const section = indexPath.section;
  NSUInteger const rowsInSection = m_cells[m_sections[section]].size();
  BOOL const isLastCell = rowsInSection - 1 == row;
  if (isLastCell)
    return YES;
  BOOL const isPenultimateCell = rowsInSection - 2 == row;
  if (!isPenultimateCell)
    return NO;
  NSIndexPath * lastIndexPath = [NSIndexPath indexPathForRow:row + 1 inSection:section];
  BOOL const isLastCellSeparator = ([self cellTypeForIndexPath:lastIndexPath] == MWMPlacePageCellTypeSpacer);
  return isLastCellSeparator;
}

#pragma mark - Fill cells with data

- (void)fillCell:(UITableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  NSString * entityValue = [self getCellValue:cellType];
  BOOL const lastCell = [self isLastCellInSection:indexPath];
  switch (cellType)
  {
    case MWMPlacePageCellTypePhoneNumber:
    {
      MWMEditorTextTableViewCell * tCell = (MWMEditorTextTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_phone_number"]
                           text:entityValue
                    placeholder:L(@"phone")
                       lastCell:lastCell];
      break;
    }
    case MWMPlacePageCellTypeWebsite:
    {
      MWMEditorTextTableViewCell * tCell = (MWMEditorTextTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_website"]
                           text:entityValue
                    placeholder:L(@"website")
                       lastCell:lastCell];
      break;
    }
    case MWMPlacePageCellTypeEmail:
    {
      MWMEditorTextTableViewCell * tCell = (MWMEditorTextTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_email"]
                           text:entityValue
                    placeholder:L(@"email")
                       lastCell:lastCell];
      break;
    }
    case MWMPlacePageCellTypeOpenHours:
    {
      MWMPlacePageOpeningHoursCell * tCell = (MWMPlacePageOpeningHoursCell *)cell;
      NSString * text = entityValue ? entityValue : L(@"opening_hours");
      [tCell configWithDelegate:self info:text lastCell:lastCell];
      break;
    }
    case MWMPlacePageCellTypeWiFi:
    {
      MWMEditorSwitchTableViewCell * tCell = (MWMEditorSwitchTableViewCell *)cell;
      BOOL const on = (entityValue != nil);
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_wifi"]
                           text:L(@"wifi")
                            on:on
                       lastCell:lastCell];
      break;
    }
    case MWMPlacePageCellTypeName:
    {
      MWMEditorTextTableViewCell * tCell = (MWMEditorTextTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:nil
                           text:entityValue
                    placeholder:L(@"name")
                       lastCell:lastCell];
      break;
    }
    case MWMPlacePageCellTypeStreet:
    {
      MWMEditorSelectTableViewCell * tCell = (MWMEditorSelectTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_adress"]
                           text:entityValue
                    placeholder:L(@"street")
                       lastCell:lastCell];
      break;
    }
    case MWMPlacePageCellTypeBuilding:
    {
      MWMEditorTextTableViewCell * tCell = (MWMEditorTextTableViewCell *)cell;
      [tCell configWithDelegate:self
                           icon:nil
                           text:entityValue
                    placeholder:L(@"building")
                       lastCell:lastCell];
      break;
    }
    case MWMPlacePageCellTypeCuisine:
    {
      MWMEditorSelectTableViewCell * tCell = (MWMEditorSelectTableViewCell *)cell;
      NSString * text = [entityValue capitalizedStringWithLocale:[NSLocale currentLocale]];
      [tCell configWithDelegate:self
                           icon:[UIImage imageNamed:@"ic_placepage_cuisine"]
                           text:text
                    placeholder:L(@"select_cuisine")
                       lastCell:lastCell];
      break;
    }
    case MWMPlacePageCellTypeSpacer:
      break;
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

- (UIView * _Nullable)tableView:(UITableView * _Nonnull)tableView viewForHeaderInSection:(NSInteger)section
{
  MWMEditorTableViewHeader * headerView =
      [[[NSBundle mainBundle] loadNibNamed:@"MWMEditorTableViewHeader"
                                     owner:nil
                                   options:nil] firstObject];
  switch (m_sections[section])
  {
    case MWMEditorSectionName:
      return nil;
    case MWMEditorSectionAddress:
      [headerView config:L(@"address")];
      break;
    case MWMEditorSectionDetails:
      [headerView config:L(@"details")];
      break;
  }
  return headerView;
}

- (CGFloat)tableView:(UITableView * _Nonnull)tableView heightForHeaderInSection:(NSInteger)section
{
  switch (m_sections[section])
  {
    case MWMEditorSectionName:
      return 0.0;
    case MWMEditorSectionAddress:
    case MWMEditorSectionDetails:
      return 36.0;
  }
}

#pragma mark - MWMPlacePageOpeningHoursCellProtocol

- (BOOL)forcedButton
{
  return YES;
}

- (BOOL)isPlaceholder
{
  return [self getCellValue:MWMPlacePageCellTypeOpenHours] == nil;
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

- (void)cell:(UITableViewCell *)cell changeText:(NSString *)changeText
{
  NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case MWMPlacePageCellTypeName:
    case MWMPlacePageCellTypePhoneNumber:
    case MWMPlacePageCellTypeWebsite:
    case MWMPlacePageCellTypeEmail:
    case MWMPlacePageCellTypeBuilding:
      [self setCell:cellType value:changeText];
      break;
    default:
      NSAssert(false, @"Invalid field for changeText");
      break;
  }
}

- (void)cell:(UITableViewCell *)cell changeSwitch:(BOOL)changeSwitch
{
  NSIndexPath * indexPath = [self.tableView indexPathForCell:cell];
  MWMPlacePageCellType const cellType = [self cellTypeForIndexPath:indexPath];
  switch (cellType)
  {
    case MWMPlacePageCellTypeWiFi:
    {
      BOOL const on = ([self getCellValue:cellType] != nil);
      if (changeSwitch != on)
        [self setCell:cellType value:changeSwitch ? @"wlan" : @""];
      break;
    }
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
  [self setCell:MWMPlacePageCellTypeOpenHours value:openingHours];
}

#pragma mark - MWMCuisineEditorProtocol

- (NSSet<NSString *> *)getCuisines
{
  return self.entity.cuisines;
}

- (void)setCuisines:(NSSet<NSString *> *)cuisines
{
  if ([[self getCuisines] isEqualToSet:cuisines])
    return;
  self.needsReload = YES;
  self.entity.cuisines = cuisines;
  // To get updated value we use [self.entity getCellValue:] not [self getCellValue:]
  NSString * updatedValue = [self.entity getCellValue:MWMPlacePageCellTypeCuisine];
  [self setCell:MWMPlacePageCellTypeCuisine value:updatedValue];
}

#pragma mark - MWMStreetEditorProtocol

- (NSString *)getStreet
{
  return [self getCellValue:MWMPlacePageCellTypeStreet];
}

- (void)setStreet:(NSString *)street
{
  [self setCell:MWMPlacePageCellTypeStreet value:street];
}

- (NSArray<NSString *> *)getNearbyStreets
{
  return self.entity.nearbyStreets;
}

#pragma mark - Segue

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
  if ([segue.identifier isEqualToString:kOpeningHoursEditorSegue])
  {
    MWMOpeningHoursEditorViewController * dvc = segue.destinationViewController;
    dvc.openingHours = [self getCellValue:MWMPlacePageCellTypeOpenHours];
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
