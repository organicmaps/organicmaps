// Copyright (c) 2014-present, Facebook, Inc. All rights reserved.
//
// You are hereby granted a non-exclusive, worldwide, royalty-free license to use,
// copy, modify, and distribute this software in source code or binary form for use
// in connection with the web services and APIs provided by Facebook.
//
// As with any software that integrates with the Facebook platform, your use of
// this software is subject to the Facebook Developer Principles and Policies
// [http://developers.facebook.com/policy/]. This copyright notice shall be
// included in all copies or substantial portions of the software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#import "TargetConditionals.h"

#if !TARGET_OS_TV

#import "FBSDKModelManager.h"

#import "FBSDKAddressFilterManager.h"
#import "FBSDKAddressInferencer.h"
#import "FBSDKEventInferencer.h"
#import "FBSDKFeatureExtractor.h"
#import "FBSDKFeatureManager.h"
#import "FBSDKGraphRequest.h"
#import "FBSDKGraphRequestConnection.h"
#import "FBSDKSettings.h"
#import "FBSDKSuggestedEventsIndexer.h"
#import "FBSDKTypeUtility.h"
#import "FBSDKViewHierarchyMacros.h"

#define FBSDK_ML_MODEL_PATH @"models"

static NSString *const MODEL_INFO_KEY= @"com.facebook.sdk:FBSDKModelInfo";
static NSString *const ASSET_URI_KEY = @"asset_uri";
static NSString *const RULES_URI_KEY = @"rules_uri";
static NSString *const THRESHOLDS_KEY = @"thresholds";
static NSString *const USE_CASE_KEY = @"use_case";
static NSString *const VERSION_ID_KEY = @"version_id";
static NSString *const MODEL_DATA_KEY = @"data";
static NSString *const ADDRESS_FILTERING_KEY = @"DATA_DETECTION_ADDRESS";

static NSString *_directoryPath;
static NSMutableDictionary<NSString *, id> *_modelInfo;

@implementation FBSDKModelManager

+ (void)enable
{
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    NSString *languageCode = [[NSLocale currentLocale] objectForKey:NSLocaleLanguageCode];
    // If the languageCode could not be fetched successfully, it's regarded as "en" by default.
    if (languageCode && ![languageCode isEqualToString:@"en"]) {
      return;
    }

    NSString *dirPath = [NSTemporaryDirectory() stringByAppendingPathComponent:FBSDK_ML_MODEL_PATH];
    if (![[NSFileManager defaultManager] fileExistsAtPath:dirPath]) {
      [[NSFileManager defaultManager] createDirectoryAtPath:dirPath withIntermediateDirectories:NO attributes:NULL error:NULL];
    }
    _directoryPath = dirPath;

    // fetch api
    FBSDKGraphRequest *request = [[FBSDKGraphRequest alloc]
                                  initWithGraphPath:[NSString stringWithFormat:@"%@/model_asset", [FBSDKSettings appID]]];

    [request startWithCompletionHandler:^(FBSDKGraphRequestConnection *connection, id result, NSError *error) {
      if (error) {
        return;
      }
      NSDictionary<NSString *, id> *resultDictionary = [FBSDKTypeUtility dictionaryValue:result];
      NSDictionary<NSString *, id> *modelInfo = [self convertToDictionary:resultDictionary[MODEL_DATA_KEY]];
      if (!modelInfo) {
        return;
      }
      // update cache
      [[NSUserDefaults standardUserDefaults] setObject:modelInfo forKey:MODEL_INFO_KEY];

      [FBSDKFeatureManager checkFeature:FBSDKFeatureSuggestedEvents completionBlock:^(BOOL enabled) {
        if (enabled) {
          [self getModelAndRules:SUGGEST_EVENT_KEY handler:^(BOOL success){
            if (success) {
              [FBSDKEventInferencer loadWeights];
              [FBSDKFeatureExtractor loadRules];
              [FBSDKSuggestedEventsIndexer enable];
            }
          }];
        }
      }];
      [FBSDKFeatureManager checkFeature:FBSDKFeaturePIIFiltering completionBlock:^(BOOL enabled) {
        if (enabled) {
          [self getModelAndRules:ADDRESS_FILTERING_KEY handler:^(BOOL success){
            if (success) {
              [FBSDKAddressInferencer loadWeights];
              [FBSDKAddressInferencer initializeDenseFeature];
              [FBSDKAddressFilterManager enable];
            }
          }];
        }
      }];
    }];
  });
}

+ (void)getModelAndRules:(NSString *)useCaseKey
                 handler:(FBSDKDownloadCompletionBlock)handler
{
  dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  dispatch_group_t group = dispatch_group_create();
  _modelInfo = [[NSUserDefaults standardUserDefaults] objectForKey:MODEL_INFO_KEY];
  if (!_modelInfo || !_directoryPath) {
    if (handler) {
      handler(NO);
      return;
    }
  }
  NSDictionary<NSString *, id> *model = [_modelInfo objectForKey:useCaseKey];

  if (!model) {
    if (handler) {
      handler(NO);
      return;
    }
  }

  // clear old model files
  NSArray<NSString *> *files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:_directoryPath error:nil];
  NSString *prefixWithVersion = [NSString stringWithFormat:@"%@_%@", useCaseKey, model[VERSION_ID_KEY]];

  for (NSString *file in files) {
    if ([file hasPrefix:useCaseKey] && ![file hasPrefix:prefixWithVersion]) {
      [[NSFileManager defaultManager] removeItemAtPath:[_directoryPath stringByAppendingPathComponent:file] error:nil];
    }
  }

  // download model asset
  NSString *assetUrlString = [model objectForKey:ASSET_URI_KEY];
  NSString *assetFilePath;
  if (assetUrlString.length > 0) {
    assetFilePath = [_directoryPath stringByAppendingPathComponent:[NSString stringWithFormat:@"%@_%@.weights", useCaseKey, model[VERSION_ID_KEY]]];
    [self download:assetUrlString filePath:assetFilePath queue:queue group:group];
  }

  // download rules
  NSString *rulesUrlString = [model objectForKey:RULES_URI_KEY];
  NSString *rulesFilePath;
  // rules are optional and rulesUrlString may be empty
  if (rulesUrlString.length > 0) {
    rulesFilePath = [_directoryPath stringByAppendingPathComponent:[NSString stringWithFormat:@"%@_%@.rules", useCaseKey, model[VERSION_ID_KEY]]];
    [self download:rulesUrlString filePath:rulesFilePath queue:queue group:group];
  }
  dispatch_group_notify(group, dispatch_get_main_queue(), ^{
    if (handler) {
      if ([[NSFileManager defaultManager] fileExistsAtPath:assetFilePath] && (!rulesUrlString || (rulesUrlString && [[NSFileManager defaultManager] fileExistsAtPath:rulesFilePath]))) {
          handler(YES);
          return;
      }
      handler(NO);
    }
  });
}

+ (void)download:(NSString *)urlString
        filePath:(NSString *)filePath
           queue:(dispatch_queue_t)queue
           group:(dispatch_group_t)group
{
  if (!filePath || [[NSFileManager defaultManager] fileExistsAtPath:filePath]) {
    return;
  }
  dispatch_group_async(group, queue, ^{
    NSURL *url = [NSURL URLWithString:urlString];
    NSData *urlData = [NSData dataWithContentsOfURL:url];
    if (urlData) {
      [urlData writeToFile:filePath atomically:YES];
    }
  });
}

+ (nullable NSMutableDictionary<NSString *, id> *)convertToDictionary:(NSArray<NSDictionary<NSString *, id> *> *)models
{
  if ([models count] == 0) {
    return nil;
  }
  NSMutableDictionary<NSString *, id> *modelInfo = [NSMutableDictionary dictionary];
  for (NSDictionary<NSString *, id> *model in models) {
    if (model[USE_CASE_KEY]) {
      [modelInfo addEntriesFromDictionary:@{model[USE_CASE_KEY]:model}];
    }
  }
  return modelInfo;
}

+ (nullable NSDictionary *)getRules
{
  NSDictionary<NSString *, id> *cachedModelInfo = [[NSUserDefaults standardUserDefaults] objectForKey:MODEL_INFO_KEY];
  if (!cachedModelInfo) {
    return nil;
  }
  NSDictionary<NSString *, id> *model = [cachedModelInfo objectForKey:SUGGEST_EVENT_KEY];
  if (model && model[VERSION_ID_KEY]) {
    NSString *filePath = [_directoryPath stringByAppendingPathComponent:[NSString stringWithFormat:@"%@_%@.rules", SUGGEST_EVENT_KEY, model[VERSION_ID_KEY]]];
    if (filePath) {
      NSData *ruelsData = [NSData dataWithContentsOfFile:filePath];
      NSDictionary *rules = [NSJSONSerialization JSONObjectWithData:ruelsData options:0 error:nil];
      return rules;
    }
  }
  return nil;
}

+ (nullable NSString *)getWeightsPath:(NSString *_Nonnull)useCaseKey
{
  NSDictionary<NSString *, id> *cachedModelInfo = [[NSUserDefaults standardUserDefaults] objectForKey:MODEL_INFO_KEY];
  if (!cachedModelInfo || !_directoryPath) {
    return nil;
  }
  NSDictionary<NSString *, id> *model = [cachedModelInfo objectForKey:useCaseKey];
  if (model && model[VERSION_ID_KEY]) {
    return [_directoryPath stringByAppendingPathComponent:[NSString stringWithFormat:@"%@_%@.weights", useCaseKey, model[VERSION_ID_KEY]]];
  }
  return nil;
}

@end

#endif
