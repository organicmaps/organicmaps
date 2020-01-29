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

#import "FBSDKEventInferencer.h"

#import <Foundation/Foundation.h>

#import "FBSDKFeatureExtractor.h"
#import "FBSDKModelManager.h"
#import "FBSDKModelRuntime.hpp"
#import "FBSDKModelUtility.h"
#import "FBSDKViewHierarchyMacros.h"

#include<stdexcept>

static NSString *const MODEL_INFO_KEY= @"com.facebook.sdk:FBSDKModelInfo";
static NSString *const THRESHOLDS_KEY = @"thresholds";
static NSString *const SUGGESTED_EVENT[4] = {@"fb_mobile_add_to_cart", @"fb_mobile_complete_registration", @"other", @"fb_mobile_purchase"};
static NSDictionary<NSString *, NSString *> *const DEFAULT_PREDICTION = @{SUGGEST_EVENT_KEY: SUGGESTED_EVENTS_OTHER};
static NSDictionary<NSString *, NSArray *> *const WEIGHTS_INFO = @{@"embed.weight" : @[@(256), @(64)],
                                                                   @"convs.0.weight" : @[@(32), @(64), @(2)],
                                                                   @"convs.0.bias" : @[@(32)],
                                                                   @"convs.1.weight" : @[@(32), @(64), @(3)],
                                                                   @"convs.1.bias" : @[@(32)],
                                                                   @"convs.2.weight" : @[@(32), @(64), @(5)],
                                                                   @"convs.2.bias" : @[@(32)],
                                                                   @"fc1.weight": @[@(128), @(126)],
                                                                   @"fc1.bias": @[@(128)],
                                                                   @"fc2.weight": @[@(64), @(128)],
                                                                   @"fc2.bias": @[@(64)],
                                                                   @"fc3.weight": @[@(4), @(64)],
                                                                   @"fc3.bias": @[@(4)]};

static std::unordered_map<std::string, mat::MTensor> _weights;

@implementation FBSDKEventInferencer : NSObject

+ (void)loadWeights
{
  NSString *path = [FBSDKModelManager getWeightsPath:SUGGEST_EVENT_KEY];
  if (!path) {
    return;
  }
  NSData *latestData = [NSData dataWithContentsOfFile:path
                                              options:NSDataReadingMappedIfSafe
                                                error:nil];
  if (!latestData) {
    return;
  }
  std::unordered_map<std::string, mat::MTensor> weights = [self loadWeights:latestData];
  if ([self validateWeights:weights]) {
    _weights = weights;
  }
}

+ (bool)validateWeights: (std::unordered_map<std::string, mat::MTensor>) weights
{
  if (WEIGHTS_INFO.count != weights.size()) {
    return false;
  }
  try {
    for (NSString *key in WEIGHTS_INFO) {
      if (weights.count(std::string([key UTF8String])) == 0) {
        return false;
      }
      mat::MTensor tensor = weights[std::string([key UTF8String])];
      const std::vector<int64_t>& actualSize = tensor.sizes();
      NSArray *expectedSize = WEIGHTS_INFO[key];
      if (actualSize.size() != expectedSize.count) {
        return false;
      }
      for (int i = 0; i < expectedSize.count; i++) {
        if((int)actualSize[i] != (int)[expectedSize[i] intValue]) {
          return false;
        }
      }
    }
  } catch (const std::exception &e) {
    return false;
  }
  return true;
}

+ (std::unordered_map<std::string, mat::MTensor>)loadWeights:(NSData *)weightsData{
  std::unordered_map<std::string,  mat::MTensor> weights;
  try {
    const void *data = weightsData.bytes;
    NSUInteger totalLength =  weightsData.length;

    int totalFloats = 0;
    if (weightsData.length < 4) {
      // Make sure data length is valid
      return weights;
    }

    int length;
    memcpy(&length, data, 4);
    if (length + 4 > totalLength) {
      // Make sure data length is valid
      return weights;
    }

    char *json = (char *)data + 4;
    NSDictionary<NSString *, id> *info = [NSJSONSerialization JSONObjectWithData:[NSData dataWithBytes:json length:length]
                                                                         options:0
                                                                           error:nil];
    NSArray<NSString *> *keys = [[info allKeys] sortedArrayUsingComparator:^NSComparisonResult(NSString *key1, NSString *key2) {
      return [key1 compare:key2];
    }];

    float *floats = (float *)(json + length);
    for (NSString *key in keys) {
      std::string s_name([key UTF8String]);

      std::vector<int64_t> v_shape;
      NSArray<NSString *> *shape = [info objectForKey:key];
      int count = 1;
      for (NSNumber *_s in shape) {
        int i = [_s intValue];
        v_shape.push_back(i);
        count *= i;
      }

      totalFloats += count;

      if ((4 + length + totalFloats * 4) > totalLength) {
        // Make sure data length is valid
        break;
      }
      mat::MTensor tensor = mat::mempty(v_shape);
      float *tensor_data = tensor.data<float>();
      memcpy(tensor_data, floats, sizeof(float) * count);
      floats += count;

      weights[s_name] = tensor;
    }
  } catch(const std::exception &e) {}

  return weights;
}

+ (NSDictionary<NSString *, NSString *> *)predict:(NSString *)buttonText
                                         viewTree:(NSMutableDictionary *)viewTree
                                          withLog:(BOOL)isPrint
{
  if (buttonText.length == 0 || _weights.size() == 0) {
    return DEFAULT_PREDICTION;
  }
  try {
    // Get bytes tensor
    NSString *textFeature = [FBSDKModelUtility normalizeText:[FBSDKFeatureExtractor getTextFeature:buttonText withScreenName:viewTree[@"screenname"]]];
    if (textFeature.length == 0) {
      return DEFAULT_PREDICTION;
    }
    const char *bytes = [textFeature UTF8String];
    if ((int)strlen(bytes) == 0) {
      return DEFAULT_PREDICTION;
    }

    // Get dense tensor
    std::vector<int64_t> dense_tensor_shape;
    dense_tensor_shape.push_back(1);
    dense_tensor_shape.push_back(30);
    mat::MTensor dense_tensor = mat::mempty(dense_tensor_shape);
    float *dense_tensor_data = dense_tensor.data<float>();
    float *dense_data = [FBSDKFeatureExtractor getDenseFeatures:viewTree];
    if (!dense_data) {
      return DEFAULT_PREDICTION;
    }

    NSMutableDictionary<NSString *, NSString *> *result = [[NSMutableDictionary alloc] init];

    // Get dense feature string
    NSMutableArray *denseDataArray = [NSMutableArray array];
    for (int i=0; i < 30; i++) {
      [denseDataArray addObject:[NSNumber numberWithFloat: dense_data[i]]];
    }
    [result setObject:[denseDataArray componentsJoinedByString:@","] forKey:DENSE_FEATURE_KEY];

    memcpy(dense_tensor_data, dense_data, sizeof(float) * 30);
    free(dense_data);
    float *res = mat1::predictOnText(bytes, _weights, dense_tensor_data);
    NSMutableDictionary<NSString *, id> *modelInfo = [[NSUserDefaults standardUserDefaults] objectForKey:MODEL_INFO_KEY];
    if (!modelInfo) {
      return DEFAULT_PREDICTION;
    }
    NSDictionary<NSString *, id> * suggestedEventModelInfo = [modelInfo objectForKey:SUGGEST_EVENT_KEY];
    if (!suggestedEventModelInfo) {
      return DEFAULT_PREDICTION;
    }
    NSMutableArray *thresholds = [suggestedEventModelInfo objectForKey:THRESHOLDS_KEY];
    if (thresholds.count < 4) {
      return DEFAULT_PREDICTION;
    }

    for (int i = 0; i < thresholds.count; i++){
      if ((float)res[i] >= (float)[thresholds[i] floatValue]) {
        [result setObject:SUGGESTED_EVENT[i] forKey:SUGGEST_EVENT_KEY];
        return result;
      }
    }
  } catch (const std::exception &e) {}
  return DEFAULT_PREDICTION;
}

@end

#endif
