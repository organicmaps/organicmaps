#import <Foundation/Foundation.h>

#define SETTINGS_REDBUTTON_URL_KEY "RedbuttonUrl"

@interface SearchBannerChecker : NSObject
{
  id m_searchButton;
  id m_downloadButton;
}
-(void) checkForBannerAndFixSearchButton:(id)searchButton
                       andDownloadButton:(id)downloadButton;
@end