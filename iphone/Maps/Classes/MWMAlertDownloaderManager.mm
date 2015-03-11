//
//  MWMMapsDownloaderDelegate.m
//  Maps
//
//  Created by v.mikhaylenko on 07.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlertDownloaderManager.h"
#include "Framework.h"

@implementation MWMAlertDownloaderManager {
  vector<storage::TIndex> _indexes;
}

- (instancetype)initWithMapsIndexes:(const vector<storage::TIndex> &)indexes {
  self = [super init];
  if (self) {
    _indexes = indexes;
    self.countryName = [NSString stringWithUTF8String:GetFramework().GetCountryTree().GetActiveMapLayout().GetFormatedCountryName(_indexes[0]).c_str()];
    self.size = GetFramework().GetCountryTree().GetActiveMapLayout().GetCountrySize(_indexes[0], storage::TMapOptions::EMapWithCarRouting).second/(1024 * 1024);
  }
  return self;
}

- (void)downloadMaps {
  GetFramework().GetCountryTree().GetActiveMapLayout().DownloadMap(_indexes[0], storage::TMapOptions::EMapWithCarRouting);
}

@end
