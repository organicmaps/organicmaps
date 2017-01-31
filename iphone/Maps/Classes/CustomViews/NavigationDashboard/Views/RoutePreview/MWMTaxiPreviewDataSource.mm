#import "MWMTaxiPreviewDataSource.h"
#import "MWMCommon.h"
#import "MWMNetworkPolicy.h"
#import "MWMRoutePoint.h"
#import "MWMTaxiPreviewCell.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "geometry/mercator.hpp"

#include "partners_api/uber_api.hpp"

namespace
{
CGFloat const kPageControlHeight = 6;

}  // namespace

@interface MWMTaxiCollectionView ()

@property(nonatomic) UIPageControl * pageControl;

@end

@implementation MWMTaxiCollectionView

- (void)setNumberOfPages:(NSUInteger)numberOfPages
{
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
  vector<Product> m_products;
  ms::LatLon m_from;
  ms::LatLon m_to;
  uint64_t m_requestId;
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
    [collectionView registerWithCellClass:[MWMTaxiPreviewCell class]];
  }
  return self;
}

- (void)dealloc
{
  MWMTaxiCollectionView * cv = self.collectionView;
  cv.dataSource = nil;
  cv.delegate = nil;
  self.collectionView = nil;
}

- (void)requestTaxiFrom:(MWMRoutePoint *)from
                     to:(MWMRoutePoint *)to
             completion:(MWMVoidBlock)completion
                failure:(MWMStringBlock)failure
{
  NSAssert(completion && failure, @"Completion and failure blocks must be not nil!");
  m_products.clear();
  m_from = routePointLatLon(from);
  m_to = routePointLatLon(to);
  auto cv = self.collectionView;
  cv.hidden = YES;
  cv.pageControl.hidden = YES;

  network_policy::CallPartnersApi(
      [self, completion, failure](platform::NetworkPolicy const & canUseNetwork) {
        auto const api = GetFramework().GetUberApi(canUseNetwork);
        if (!api)
        {
          failure(L(@"dialog_taxi_error"));
          return;
        }

        auto success = [self, completion](vector<Product> const & products,
                                          uint64_t const requestId) {
          if (self->m_requestId != requestId)
            return;
          runAsyncOnMainQueue([self, completion, products] {

            self->m_products = products;
            auto cv = self.collectionView;
            cv.hidden = NO;
            cv.pageControl.hidden = NO;
            cv.numberOfPages = self->m_products.size();
            [cv reloadData];
            cv.contentOffset = {};
            cv.currentPage = 0;
            completion();
          });

        };
        auto error = [self, failure](uber::ErrorCode const code, uint64_t const requestId) {
          if (self->m_requestId != requestId)
            return;
          runAsyncOnMainQueue(^{
            switch (code)
            {
            case uber::ErrorCode::NoProducts: failure(L(@"taxi_not_found")); break;
            case uber::ErrorCode::RemoteError: failure(L(@"dialog_taxi_error")); break;
            }
          });
        };
        m_requestId = api->GetAvailableProducts(m_from, m_to, success, error);
      },
      true /* force */);
}

- (BOOL)isTaxiInstalled
{
  // TODO(Vlad): Not the best solution, need to store url's scheme of product in the uber::Product
  // instead of just "uber://".
  NSURL * url = [NSURL URLWithString:@"uber://"];
  return [[UIApplication sharedApplication] canOpenURL:url];
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
  Class cls = [MWMTaxiPreviewCell class];
  auto cell = static_cast<MWMTaxiPreviewCell *>(
      [collectionView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
  [cell configWithProduct:m_products[indexPath.row]];

  return cell;
}

#pragma mark - UIScrollViewDelegate

- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView
{
  auto const offset = MAX(0, scrollView.contentOffset.x);
  auto const itemIndex = static_cast<size_t>(offset / scrollView.width);
  [self.collectionView setCurrentPage:itemIndex];
}

@end
