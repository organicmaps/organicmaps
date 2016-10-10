#import "MWMRoutePoint.h"
#import "MWMTaxiPreviewCell.h"
#import "MWMTaxiPreviewDataSource.h"

#include "geometry/mercator.hpp"

#include "partners_api/uber_api.hpp"

static CGFloat const kPageControlHeight = 6;

@interface MWMTaxiCollectionView ()

@property(nonatomic) UIPageControl * pageControl;

@end

@implementation MWMTaxiCollectionView

- (void)setNumberOfPages:(NSUInteger)numberOfPages
{
  _numberOfPages = numberOfPages;
  [self.pageControl setNumberOfPages:numberOfPages];
  [self.pageControl setCurrentPage:0];
}

- (void)setCurrentPage:(NSUInteger)currentPage
{
  [self.pageControl setCurrentPage:currentPage];
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  self.pageControl.height = kPageControlHeight;
  self.pageControl.width = self.width;
  self.pageControl.maxY = self.height - kPageControlHeight;
  self.pageControl.midX = self.center.x;
}

- (UIPageControl *)pageControl
{
  if (!_pageControl)
  {
    _pageControl = [[UIPageControl alloc] init];
    [self.superview addSubview:_pageControl];
  }
  return _pageControl;
}

@end

using namespace uber;

@interface MWMTaxiPreviewDataSource() <UICollectionViewDataSource, UICollectionViewDelegate>
{
  Api m_api;
  vector<Product> m_products;
  ms::LatLon m_from;
  ms::LatLon m_to;
}

@property(weak, nonatomic) MWMTaxiCollectionView * collectionView;
@property(nonatomic) BOOL isNeedToConstructURLs;

@end

@implementation MWMTaxiPreviewDataSource

- (instancetype)initWithCollectionView:(MWMTaxiCollectionView *)collectionView
{
  self = [super init];
  if (self)
  {
    _collectionView = static_cast<MWMTaxiCollectionView *>(collectionView);
    collectionView.dataSource = self;
    collectionView.delegate = self;
    collectionView.showsVerticalScrollIndicator = NO;
    collectionView.showsHorizontalScrollIndicator = NO;
    auto name = [MWMTaxiPreviewCell className];
    [collectionView registerNib:[UINib nibWithNibName:name bundle:nil] forCellWithReuseIdentifier:name];
  }
  return self;
}

- (void)requestTaxiFrom:(MWMRoutePoint const &)from
                                  to:(MWMRoutePoint const &)to
                          completion:(TMWMVoidBlock)completion
                             failure:(TMWMVoidBlock)failure
{
  NSAssert(completion && failure, @"Completion and failure blocks must be not nil!");
  m_products.clear();
  m_from = MercatorBounds::ToLatLon(from.Point());
  m_to = MercatorBounds::ToLatLon(to.Point());
  self.collectionView.hidden = YES;
  m_api.GetAvailableProducts(m_from, m_to, [self, completion, failure](vector<Product> const & products,
                                                                  size_t const requestId)
  {
    if (products.empty())
    {
      dispatch_async(dispatch_get_main_queue(), ^{
        failure();
      });
      return;
    }

    m_products = products;
    dispatch_async(dispatch_get_main_queue(), ^{
      auto cv = self.collectionView;
      cv.hidden = NO;
      cv.numberOfPages = self->m_products.size();
      [cv reloadData];
      cv.contentOffset = {};
      completion();
    });
  });
}

- (BOOL)isTaxiInstalled
{
  // TODO(Vlad): Not the best solution, need to store url's scheme of product in the uber::Product
  // instead of just "uber://".
  return [[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString:@"uber://"]];
}

- (NSURL *)taxiURL;
{
  if (m_products.empty())
    return nil;

  auto const index = [self.collectionView indexPathsForVisibleItems].firstObject.row;
  auto const productId = m_products[index].m_productId;
  auto const links = Api::GetRideRequestLinks(productId, m_from, m_to);

  return [NSURL URLWithString:self.isTaxiInstalled ? @(links.m_deepLink.c_str()) :
                                                     @(links.m_universalLink.c_str())];
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
  return m_products.size();
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
  MWMTaxiPreviewCell * cell = [collectionView dequeueReusableCellWithReuseIdentifier:[MWMTaxiPreviewCell className]
                                                                        forIndexPath:indexPath];
  [cell configWithProduct:m_products[indexPath.row]];

  return cell;
}

#pragma mark - UIScrollViewDelegate

- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView
{
  auto const offset = MAX(0, scrollView.contentOffset.x);
  NSUInteger const itemIndex = offset / scrollView.width;
  [self.collectionView setCurrentPage:itemIndex];
}

@end
