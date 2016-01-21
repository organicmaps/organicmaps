#import "MWMSearchCategoriesManager.h"
#import "MWMSearchHistoryManager.h"
#import "MWMSearchTabbedCollectionViewCell.h"
#import "MWMSearchTabbedViewController.h"
#import "MWMSearchTabbedViewLayout.h"
#import "MWMSearchTabbedViewProtocol.h"
#import "Statistics.h"

#include "Framework.h"

static NSString * const kCollectionCell = @"MWMSearchTabbedCollectionViewCell";

typedef NS_ENUM(NSInteger, MWMSearchTabbedViewCell)
{
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

@interface MWMSearchTabbedViewController () <UICollectionViewDataSource, UIScrollViewDelegate>

@property (weak, nonatomic) IBOutlet UICollectionView * tablesCollectionView;

@property (weak, nonatomic) MWMSearchTabButtonsView * selectedButton;

@property (nonatomic) MWMSearchHistoryManager * historyManager;
@property (nonatomic) MWMSearchCategoriesManager * categoriesManager;

@property (nonatomic) BOOL isRotating;
@property (nonatomic) NSInteger selectedButtonTag;

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

- (void)refresh
{
  [self.view refresh];
}

- (void)resetSelectedTab
{
  self.selectedButtonTag = GetFramework().GetLastSearchQueries().empty() && !self.historyManager.isRouteSearchMode ? 1 : 0;
}

- (void)viewWillAppear:(BOOL)animated
{
  [self.tablesCollectionView reloadData];
  [self resetSelectedTab];
  [self refreshScrollPosition];
  [super viewWillAppear:animated];
}

#pragma mark - Layout

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
                                duration:(NSTimeInterval)duration
{
  self.isRotating = YES;
  [UIView animateWithDuration:duration animations:^
  {
    [self refreshScrollPosition];
  }
  completion:^(BOOL finished)
  {
    [self refreshScrollPosition];
    self.isRotating = NO;
  }];
}

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  self.isRotating = YES;
  [coordinator animateAlongsideTransition:^(id<UIViewControllerTransitionCoordinatorContext> context)
  {
    [self refreshScrollPosition];
  }
  completion:^(id<UIViewControllerTransitionCoordinatorContext> context)
  {
    self.isRotating = NO;
  }];
}

#pragma mark - Setup

- (void)setupCollectionView
{
  [self.tablesCollectionView registerNib:[UINib nibWithNibName:kCollectionCell bundle:nil]
              forCellWithReuseIdentifier:kCollectionCell];
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
  dispatch_async(dispatch_get_main_queue(), ^
  {
    [self.tablesCollectionView
        scrollToItemAtIndexPath:[NSIndexPath indexPathForItem:sender.tag inSection:0]
               atScrollPosition:UICollectionViewScrollPositionCenteredHorizontally
                       animated:YES];
  });
}

- (void)updateScrollPosition:(CGFloat)position
{
  self.scrollIndicatorOffset.constant = nearbyint(position);
  CGFloat const btnMid = position + 0.5 * self.scrollIndicator.width;
  if (self.selectedButton && isOffsetInButton(btnMid, self.selectedButton))
    return;
  [self.tabButtons enumerateObjectsUsingBlock:^(MWMSearchTabButtonsView * btn, NSUInteger idx, BOOL *stop)
  {
    if (isOffsetInButton(btnMid, btn))
    {
      switch (btn.tag)
      {
      case MWMSearchTabbedViewCellHistory:
        [[Statistics instance] logEvent:kStatEventName(kStatSearch, kStatSelectTab)
                         withParameters:@{kStatValue : kStatHistory}];
        break;
      case MWMSearchTabbedViewCellCategories:
        [[Statistics instance] logEvent:kStatEventName(kStatSearch, kStatSelectTab)
                         withParameters:@{kStatValue : kStatCategories}];
        break;
      default:
        break;
      }
      self.selectedButton = btn;
      *stop = YES;
    }
  }];
}

- (void)refreshScrollPosition
{
  self.scrollIndicatorOffset.constant = nearbyint(self.selectedButton.minX);
  [self tabButtonPressed:self.selectedButton];
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
  MWMSearchTabbedCollectionViewCell * cell =
      [collectionView dequeueReusableCellWithReuseIdentifier:kCollectionCell
                                                forIndexPath:indexPath];
  MWMSearchTabbedViewCell cellType = static_cast<MWMSearchTabbedViewCell>(indexPath.item);
  switch (cellType)
  {
    case MWMSearchTabbedViewCellHistory:
      [self.historyManager attachCell:cell];
      break;
    case MWMSearchTabbedViewCellCategories:
      [self.categoriesManager attachCell:cell];
      break;
    default:
      break;
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
  [self.tabButtons enumerateObjectsUsingBlock:^(MWMSearchTabButtonsView * btn, NSUInteger idx, BOOL *stop)
  {
    btn.selected = NO;
  }];
  _selectedButton = selectedButton;
  selectedButton.selected = YES;
}

- (void)setDelegate:(id<MWMSearchTabbedViewProtocol>)delegate
{
  _delegate = self.categoriesManager.delegate = self.historyManager.delegate = delegate;
}

- (void)setSelectedButtonTag:(NSInteger)selectedButtonTag
{
  [self.tabButtons enumerateObjectsUsingBlock:^(MWMSearchTabButtonsView * btn, NSUInteger idx, BOOL * stop)
  {
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

- (NSInteger)selectedButtonTag
{
  return self.selectedButton.tag;
}

@end
