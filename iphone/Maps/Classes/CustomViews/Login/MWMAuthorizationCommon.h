
#include "editor/osm_auth.hpp"

typedef NS_OPTIONS(NSUInteger, MWMAuthorizationButtonType)
{
  MWMAuthorizationButtonTypeGoogle,
  MWMAuthorizationButtonTypeFacebook,
  MWMAuthorizationButtonTypeOSM
};

UIColor * MWMAuthorizationButtonBackgroundColor(MWMAuthorizationButtonType type);
void MWMAuthorizationConfigButton(UIButton * btn, MWMAuthorizationButtonType type);

// Deletes any stored credentials if called with empty key or secret.
void MWMAuthorizationStoreCredentials(osm::TKeySecret const & keySecret);
BOOL MWMAuthorizationHaveCredentials();
// Returns empty key and secret if user has not beed authorized.
osm::TKeySecret MWMAuthorizationGetCredentials();

void MWMAuthorizationSetUserSkip();
BOOL MWMAuthorizationIsUserSkip();
void MWMAuthorizationSetNeedCheck(BOOL needCheck);
BOOL MWMAuthorizationIsNeedCheck();
