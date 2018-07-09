#import "MWMUGCHelper.h"

#include "Framework.h"

@implementation MWMUGCHelper

+ (void)uploadEdits:(void (^)(UIBackgroundFetchResult))completionHandler
{
  auto const lambda = [completionHandler](bool isSuccessful) {
    completionHandler(isSuccessful ? UIBackgroundFetchResultNewData
                                   : UIBackgroundFetchResultFailed);
  };

  GetFramework().UploadUGC(lambda);
}

@end
