#import "MWMSearchTabbedViewController.h"
#import "MWMCommon.h"
#import "MWMSearchCategoriesManager.h"
#import "MWMSearchHistoryManager.h"
#import "MWMSearchTabbedViewLayout.h"
#import "SwiftBridge.h"

#include "Framework.h"

namespace
{
NSString * const kSelectedButtonTagKey = @"MWMSearchTabbedCollectionViewSelectedButtonTag";
}  // namespace

typedef NS_ENUM(NSInteger, MWMSearchTabbedViewCell) {
  MWMSearchTabbedViewCellHistory,
  MWMSearchTabbedViewCellCategories,
  MWMSearchTabbedViewCellCount
};

BOOL isOffsetInButton(CGFloat offset, MWMSearchTabButtonsView * button)
{
  CGRect const frame = button.frame;
  CGFloat const left = frame.origin.x;
  CGFloat const right = left + frame.size.width;
  return left <= offset && offset <= right;
}

@interface MWMSearchTabbedViewController ()<UICollectionViewDataSource, UIScrollViewDelegate>

@property(weak, nonatomic) IBOutlet UICollectionView * tablesCollectionView;

@property(weak, nonatomic) MWMSearchTabButtonsView * selectedButton;

@property(nonatomic) MWMSearchHistoryManager * historyManager;
@property(nonatomic) MWMSearchCategoriesManager * categoriesManager;

@property(nonatomic) BOOL isRotating;
@property(nonatomic) NSInteger selectedButtonTag;

@end

@implementation MWMSearchTabbedViewController

- (instancetype)init
{
  self = [super init];
  if (self)
    [self setupDataSources];
  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self setupCollectionView];
  [self resetSelectedTab];
}

- (void)mwm_refreshUI { [self.view mwm_refreshUI]; }
- (void)resetSelectedTab
{
  if (GetFramework().GetLastSearchQueries().empty())
    self.selectedButtonTag = 1;
  else
    self.selectedButtonTag =
        [NSUserDefaults.standardUserDefaults integerForKey:kSelectedButtonTagKey];
}

- (void)resetCategories { [self.categoriesManager resetCategories]; }
- (void)viewWillAppear:(BOOL)animated
{
  self.scrollIndicator.hidden = YES;
  [self.tablesCollectionView reloadData];
  [self resetSelectedTab];
  [self refreshScrollPosition];
  [super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
  self.scrollIndicator.hidden = NO;
  [super viewDidAppear:animated];
}

#pragma mark - Layout

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
  self.isRotating = YES;
  [coordinator
      animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        [self refreshScrollPosition];
      }
      completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        self.isRotating = NO;
      }];
}

#pragma mark - Setup

- (void)setupCollectionView
{
  [self.tablesCollectionView registerWithCellClass:[MWMSearchTabbedCollectionViewCell class]];
  ((MWMSearchTabbedViewLayout *)self.tablesCollectionView.collectionViewLayout).tablesCount =
      MWMSearchTabbedViewCellCount;
}

- (void)setupDataSources
{
  self.categoriesManager = [[MWMSearchCategoriesManager alloc] init];
  self.historyManager = [[MWMSearchHistoryManager alloc] init];
}

- (void)tabButtonPressed:(MWMSearchTabButtonsView *)sender
{
  dispatch_async(dispatch_get_main_queue(), ^{
    [self.tablesCollectionView
        scrollToItemAtIndexPath:[NSIndexPath indexPathForItem:sender.tag inSection:0]
               atScrollPosition:UICollectionViewScrollPositionCenteredHorizontally
                       animated:YES];
  });
}

- (void)updateScrollPosition:(CGFloat)position
{
  CGFloat const scrollIndicatorWidth = self.scrollIndicator.width;
  CGFloat const btnMid = position + 0.5 * scrollIndicatorWidth;
  if (isInterfaceRightToLeft())
    position = scrollIndicatorWidth - position;
  dispatch_async(dispatch_get_main_queue(), ^{
    self.scrollIndicatorOffset.constant = nearbyint(position);
  });
  MWMSearchTabButtonsView * selectedButton = self.selectedButton;
  if (selectedButton && isOffsetInButton(btnMid, selectedButton))
    return;
  [self.tabButtons
      enumerateObjectsUsingBlock:^(MWMSearchTabButtonsView * btn, NSUInteger idx, BOOL * stop) {
        if (isOffsetInButton(btnMid, btn))
        {
          switch (btn.tag)
          {
          case MWMSearchTabbedViewCellHistory:
            [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectTab)
                  withParameters:@{kStatValue : kStatHistory}];
            break;
          case MWMSearchTabbedViewCellCategories:
            [Statistics logEvent:kStatEventName(kStatSearch, kStatSelectTab)
                  withParameters:@{kStatValue : kStatCategories}];
            break;
          default: break;
          }
          self.selectedButton = btn;
          *stop = YES;
        }
      }];
}

- (void)refreshScrollPosition
{
  MWMSearchTabButtonsView * selectedButton = self.selectedButton;
  self.scrollIndicatorOffset.constant = nearbyint(selectedButton.minX);
  [self tabButtonPressed:selectedButton];
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)collectionView:(nonnull UICollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section
{
  return MWMSearchTabbedViewCellCount;
}

- (nonnull UICollectionViewCell *)collectionView:(nonnull UICollectionView *)collectionView
                          cellForItemAtIndexPath:(nonnull NSIndexPath *)indexPath
{
  auto cell = static_cast<MWMSearchTabbedCollectionViewCell *>([collectionView
      dequeueReusableCellWithCellClass:[MWMSearchTabbedCollectionViewCell class]
                             indexPath:indexPath]);
  MWMSearchTabbedViewCell cellType = static_cast<MWMSearchTabbedViewCell>(indexPath.item);
  switch (cellType)
  {
  case MWMSearchTabbedViewCellHistory: [self.historyManager attachCell:cell]; break;
  case MWMSearchTabbedViewCellCategories: [self.categoriesManager attachCell:cell]; break;
  default: break;
  }
  return cell;
}

#pragma mark - UIScrollViewDelegate

- (void)scrollViewDidScroll:(nonnull UIScrollView *)scrollView
{
  if (self.isRotating)
    return;
  CGFloat const scrollOffset = scrollView.contentOffset.x;
  CGFloat const indWidth = self.scrollIndicator.width;
  CGFloat const scrWidth = self.view.width;
  [self updateScrollPosition:scrollOffset * indWidth / scrWidth];
}

#pragma mark - Properties

- (void)setSelectedButton:(MWMSearchTabButtonsView *)selectedButton
{
  [self.tabButtons
      enumerateObjectsUsingBlock:^(MWMSearchTabButtonsView * btn, NSUInteger idx, BOOL * stop) {
        btn.selected = NO;
      }];
  _selectedButton = selectedButton;
  selectedButton.selected = YES;
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setInteger:selectedButton.tag forKey:kSelectedButtonTagKey];
  [ud synchronize];
}

- (void)setDelegate:(id<MWMSearchTabbedViewProtocol>)delegate
{
  _delegate = self.categoriesManager.delegate = self.historyManager.delegate = delegate;
}

- (void)setSelectedButtonTag:(NSInteger)selectedButtonTag
{
  [self.tabButtons
      enumerateObjectsUsingBlock:^(MWMSearchTabButtonsView * btn, NSUInteger idx, BOOL * stop) {
        if (btn.tag == selectedButtonTag)
        {
          self.selectedButton = btn;
          *stop = YES;
        }
      }];
  [self updateScrollPosition:self.selectedButton.minX];
  [self.tablesCollectionView
      scrollToItemAtIndexPath:[NSIndexPath indexPathForItem:selectedButtonTag inSection:0]
             atScrollPosition:UICollectionViewScrollPositionCenteredHorizontally
                     animated:NO];
}

- (NSInteger)selectedButtonTag { return self.selectedButton.tag; }
@end
