#import "MWMTaxiPreviewDataSource.h"
#import "MWMNetworkPolicy.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "partners_api/taxi_provider.hpp"

#include "geometry/mercator.hpp"

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
  self.pageControl.height = 8;
  self.pageControl.minY = 4;
  self.pageControl.midX = self.superview.superview.center.x;
}

- (UIPageControl *)pageControl
{
  if (!_pageControl)
  {
    _pageControl = [[UIPageControl alloc] init];
    _pageControl.backgroundColor = UIColor.clearColor;
    [self.superview addSubview:_pageControl];
  }
  return _pageControl;
}

@end

using namespace taxi;

@interface MWMTaxiPreviewDataSource() <UICollectionViewDataSource, UICollectionViewDelegate>
{
  std::vector<Product> m_products;
  ms::LatLon m_from;
  ms::LatLon m_to;
  uint64_t m_requestId;
}

@property(weak, nonatomic) MWMTaxiCollectionView * collectionView;
@property(nonatomic) BOOL isNeedToConstructURLs;
@property(nonatomic, readwrite) MWMRoutePreviewTaxiCellType type;

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
    [collectionView registerWithCellClass:[MWMRoutePreviewTaxiCell class]];
  }
  return self;
}

- (void)requestTaxiFrom:(MWMRoutePoint *)from
                     to:(MWMRoutePoint *)to
             completion:(MWMVoidBlock)completion
                failure:(MWMStringBlock)failure
{
  NSAssert(completion && failure, @"Completion and failure blocks must be not nil!");
  m_products.clear();
  m_from = ms::LatLon(from.latitude, from.longitude);
  m_to = ms::LatLon(to.latitude, to.longitude);
  auto cv = self.collectionView;
  cv.hidden = YES;
  cv.pageControl.hidden = YES;

    network_policy::CallPartnersApi(
        [self, completion, failure](platform::NetworkPolicy const & canUseNetwork) {
          auto const engine = GetFramework().GetTaxiEngine(canUseNetwork);
          if (!engine) {
            failure(L(@"dialog_taxi_error"));
            return;
          }

          auto success = [self, completion, failure](taxi::ProvidersContainer const & providers,
                                                     uint64_t const requestId) {
            dispatch_async(dispatch_get_main_queue(), [self, completion, failure, providers,
                                                       requestId] {
              if (self->m_requestId != requestId)
                return;
              if (providers.empty())
              {
                failure(L(@"taxi_no_providers"));
                [Statistics logEvent:kStatRoutingBuildTaxi
                      withParameters:@{
                        @"error" : @"No providers (Taxi isn't in the city)"
                      }];
                return;
              }
              auto const & provider = providers.front();
              auto const & products = provider.GetProducts();
              auto const type = provider.GetType();
              self->m_products = products;
              NSString * providerName = nil;
              switch (type)
              {
              case taxi::Provider::Type::Uber:
                self.type = MWMRoutePreviewTaxiCellTypeUber;
                providerName = kStatUber;
                break;
              case taxi::Provider::Type::Yandex:
                self.type = MWMRoutePreviewTaxiCellTypeYandex;
                providerName = kStatYandex;
                break;
              }
              [Statistics logEvent:kStatRoutingBuildTaxi
                    withParameters:@{
                      @"provider" : providerName
                    }];
              auto cv = self.collectionView;
              cv.hidden = NO;
              cv.pageControl.hidden = products.size() == 1;
              cv.numberOfPages = products.size();
              cv.contentOffset = {};
              cv.currentPage = 0;
              [cv reloadData];
              completion();
            });
          };

          auto error = [self, failure](taxi::ErrorsContainer const & errors,
                                       uint64_t const requestId) {
            dispatch_async(dispatch_get_main_queue(), [self, failure, errors, requestId] {
              if (self->m_requestId != requestId)
                return;
              if (errors.empty())
              {
                NSCAssert(false, @"Errors container is empty");
                return;
              }
              auto const & error = errors.front();
              auto const errorCode = error.m_code;
              auto const type = error.m_type;
              NSString * provider = nil;
              switch (type)
              {
              case taxi::Provider::Type::Uber: provider = kStatUber; break;
              case taxi::Provider::Type::Yandex: provider = kStatYandex; break;
              }
              NSString * errorValue = nil;
              switch (errorCode)
              {
              case taxi::ErrorCode::NoProducts:
                errorValue = @"No products (Taxi is in the city, but no offers)";
                failure(L(@"taxi_not_found"));
                break;
              case taxi::ErrorCode::RemoteError:
                errorValue = @"Server error (The taxi server responded with an error)";
                failure(L(@"dialog_taxi_error"));
                break;
              }
              [Statistics logEvent:kStatRoutingBuildTaxi
                    withParameters:@{
                      @"provider" : provider,
                      @"error" : errorValue
                    }];
            });
          };

          m_requestId = engine->GetAvailableProducts(m_from, m_to, success, error);
        });
}

- (BOOL)isTaxiInstalled
{
  NSURL * url;
  switch (self.type)
  {
  case MWMRoutePreviewTaxiCellTypeTaxi: return NO;
  case MWMRoutePreviewTaxiCellTypeUber: url = [NSURL URLWithString:@"uber://"]; break;
  case MWMRoutePreviewTaxiCellTypeYandex: url = [NSURL URLWithString:@"yandextaxi://"]; break;
  }
  return [UIApplication.sharedApplication canOpenURL:url];
}

- (void)taxiURL:(MWMURLBlock)block
{
  if (m_products.empty())
    return;

  auto const index = [self.collectionView indexPathsForVisibleItems].firstObject.row;
  auto const productId = m_products[index].m_productId;
  network_policy::CallPartnersApi(
    [self, productId, block](platform::NetworkPolicy const & canUseNetwork)
    {
      auto const engine = GetFramework().GetTaxiEngine(canUseNetwork);
      if (!engine)
        return;
      Provider::Type type;
      switch (self.type)
      {
        case MWMRoutePreviewTaxiCellTypeTaxi: return;
        case MWMRoutePreviewTaxiCellTypeUber: type = Provider::Type::Uber; break;
        case MWMRoutePreviewTaxiCellTypeYandex: type = Provider::Type::Yandex;  break;
      }

      auto links = engine->GetRideRequestLinks(type, productId, m_from, m_to);
      auto url = [NSURL URLWithString:self.isTaxiInstalled ? @(links.m_deepLink.c_str()) :
                  @(links.m_universalLink.c_str())];
      block(url);
    });
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
  return m_products.size();
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
  Class cls = [MWMRoutePreviewTaxiCell class];
  auto cell = static_cast<MWMRoutePreviewTaxiCell *>(
      [collectionView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
  auto const & product = m_products[indexPath.row];
  [cell configWithType:self.type
                 title:@(product.m_name.c_str())
                   eta:@(product.m_time.c_str())
                 price:@(product.m_price.c_str())
              currency:@(product.m_currency.c_str())];

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
