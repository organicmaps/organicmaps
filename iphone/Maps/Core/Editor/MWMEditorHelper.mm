#import "MWMEditorHelper.h"
#import <CoreApi/AppInfo.h>
#import "MWMAuthorizationCommon.h"

#include "editor/osm_auth.hpp"
#include "editor/osm_editor.hpp"

#include "storage/pinger.hpp"

@implementation MWMEditorHelper

+ (BOOL)hasMapEditsOrNotesToUpload
{
  return osm::Editor::Instance().HaveMapEditsOrNotesToUpload();
}

+ (BOOL)isOSMServerReachable
{
/* When the Pinger is used to check the reachability of the OSM site, it also verifies that no redirection occurred by
 comparing the request and response URL strings. However, OSM always returns the path with a trailing slash:
 https://www.openstreetmap.org/
 instead of
 https://www.openstreetmap.org (which is used when sending edits).
 As a result, the Ping check fails due to a false positive redirection. This is a workaround to avoid this issue.
 */
#ifdef DEBUG
  std::string osmServerUrl = "https://master.apis.dev.openstreetmap.org/";
#else
  std::string osmServerUrl = "https://www.openstreetmap.org/";
#endif
  uint64_t kPingTimeoutInSeconds = 1;
  auto const sorted = storage::Pinger::ExcludeUnavailableAndSortEndpoints({osmServerUrl}, kPingTimeoutInSeconds);
  return !sorted.empty();
}

+ (void)uploadEdits:(void (^)(UIBackgroundFetchResult))completionHandler
{
  if (!osm_auth_ios::AuthorizationHaveCredentials() ||
      Platform::EConnectionType::CONNECTION_NONE == Platform::ConnectionStatus())
  {
    completionHandler(UIBackgroundFetchResultFailed);
  }
  else
  {
    auto const lambda = [completionHandler](osm::Editor::UploadResult result)
    {
      switch (result)
      {
      case osm::Editor::UploadResult::Success: completionHandler(UIBackgroundFetchResultNewData); break;
      case osm::Editor::UploadResult::Error: completionHandler(UIBackgroundFetchResultFailed); break;
      case osm::Editor::UploadResult::NothingToUpload: completionHandler(UIBackgroundFetchResultNoData); break;
      }
    };
    std::string const oauthToken = osm_auth_ios::AuthorizationGetCredentials();
    osm::Editor::Instance().UploadChanges(
        oauthToken,
        {{"created_by", std::string("Organic Maps " OMIM_OS_NAME " ") + AppInfo.sharedInfo.bundleVersion.UTF8String},
         {"bundle_id", NSBundle.mainBundle.bundleIdentifier.UTF8String}},
        lambda);
  }
}

@end
