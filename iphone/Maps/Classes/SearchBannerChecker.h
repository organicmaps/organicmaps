#import <Foundation/Foundation.h>

#define SETTINGS_REDBUTTON_URL_KEY "RedbuttonUrl"

@interface SearchBannerChecker : NSObject
{
  id m_searchButton;
}
-(void) checkForBannerAndEnableButton:(id)searchButton;
@end