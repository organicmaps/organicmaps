#import "Common.h"
#import "MWMStorage.h"

#include "Framework.h"

#include "platform/platform.hpp"

@implementation MWMStorage

+ (void)downloadNode:(storage::TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController onSuccess:(TMWMVoidBlock)onSuccess
{
  [self countryId:countryId alertController:alertController performAction:^
  {
    GetFramework().Storage().DownloadNode(countryId);
    if (onSuccess)
      onSuccess();
  }];
}

+ (void)retryDownloadNode:(storage::TCountryId const &)countryId
{
  GetFramework().Storage().RetryDownloadNode(countryId);
}

+ (void)updateNode:(storage::TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController
{
  [self countryId:countryId alertController:alertController performAction:^
  {
    GetFramework().Storage().UpdateNode(countryId);
  }];
}

+ (void)deleteNode:(storage::TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController
{
  auto & f = GetFramework();
  if (f.HasUnsavedEdits(countryId))
  {
    [alertController presentUnsavedEditsAlertWithOkBlock:^
    {
       f.Storage().DeleteNode(countryId);
    }];
  }
  else
  {
    f.Storage().DeleteNode(countryId);
  }
}

+ (void)cancelDownloadNode:(storage::TCountryId const &)countryId
{
  GetFramework().Storage().CancelDownloadNode(countryId);
}

+ (void)showNode:(storage::TCountryId const &)countryId
{
  GetFramework().ShowNode(countryId);
}

+ (void)countryId:(storage::TCountryId const &)countryId alertController:(MWMAlertViewController *)alertController performAction:(TMWMVoidBlock)action
{
  switch (Platform::ConnectionStatus())
  {
    case Platform::EConnectionType::CONNECTION_NONE:
      [alertController presentNoConnectionAlert];
      break;
    case Platform::EConnectionType::CONNECTION_WIFI:
      action();
      break;
    case Platform::EConnectionType::CONNECTION_WWAN:
    {
      storage::NodeAttrs attrs;
      GetFramework().Storage().GetNodeAttrs(countryId, attrs);
      size_t const warningSizeForWWAN = 50 * MB;
      if (attrs.m_mwmSize > warningSizeForWWAN)
        [alertController presentNoWiFiAlertWithName:@(attrs.m_nodeLocalName.c_str()) okBlock:action];
      else
        action();
      break;
    }
  }
}

@end
