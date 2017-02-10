// Copyright 2004-present Facebook. All Rights Reserved.
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

#import <UIKit/UIKit.h>

@protocol FBInstreamAdViewDelegate;

@interface FBInstreamAdView : UIView

@property (nonatomic, getter=isAdValid, readonly) BOOL adValid;
@property (nonatomic, weak, nullable) id<FBInstreamAdViewDelegate> delegate;
@property (nonatomic, copy, readonly, nonnull) NSString *placementID;

- (nullable instancetype)initWithPlacementID:(nonnull NSString *)placementID NS_DESIGNATED_INITIALIZER;

- (void)loadAd;

- (BOOL)showAdFromRootViewController:(nullable UIViewController *)rootViewController;

@end

@protocol FBInstreamAdViewDelegate <NSObject>

- (void)adViewDidLoad:(nonnull FBInstreamAdView *)adView;

- (void)adViewDidEnd:(nonnull FBInstreamAdView *)adView;

- (void)adView:(nonnull FBInstreamAdView *)adView didFailWithError:(nonnull NSError *)error;

@optional

- (void)adViewDidClick:(nonnull FBInstreamAdView *)adView;

- (void)adViewWillLogImpression:(nonnull FBInstreamAdView *)adView;

@end
