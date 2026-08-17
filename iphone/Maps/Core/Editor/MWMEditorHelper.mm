#import "MWMEditorHelper.h"
#import <CoreApi/AppInfo.h>
#import "MWMAuthorizationCommon.h"

#include "editor/osm_editor.hpp"

@implementation MWMEditorHelper

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
    switch (osm::Editor::Instance().UploadChanges(
        oauthToken,
        {{"created_by", std::string("Organic Maps " OMIM_OS_NAME " ") + AppInfo.sharedInfo.bundleVersion.UTF8String},
         {"bundle_id", NSBundle.mainBundle.bundleIdentifier.UTF8String}},
        lambda))
    {
    case osm::Editor::UploadStart::Started: break;
    // Unlike Android, the result does not reschedule anything here: it only ends the background
    // task, so both reasons for not starting are reported the same way.
    case osm::Editor::UploadStart::AlreadyUploading:  // fallthrough
    case osm::Editor::UploadStart::NothingToUpload: completionHandler(UIBackgroundFetchResultNoData); break;
    }
  }
}

@end
