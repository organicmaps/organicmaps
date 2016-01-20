
#include "editor/osm_auth.hpp"

typedef NS_OPTIONS(NSUInteger, MWMAuthorizationButtonType)
{
  MWMAuthorizationButtonTypeGoogle,
  MWMAuthorizationButtonTypeFacebook,
  MWMAuthorizationButtonTypeOSM
};

UIColor * MWMAuthorizationButtonBackgroundColor(MWMAuthorizationButtonType type);
void MWMAuthorizationConfigButton(UIButton * btn, MWMAuthorizationButtonType type);

void MWMAuthorizationStoreCredentials(osm::TKeySecret const & keySecret);
BOOL MWMAuthorizationHaveCredentials();

void MWMAuthorizationSetUserSkip();
BOOL MWMAuthorizationIsUserSkip();
void MWMAuthorizationSetNeedCheck(BOOL needCheck);
BOOL MWMAuthorizationIsNeedCheck();
