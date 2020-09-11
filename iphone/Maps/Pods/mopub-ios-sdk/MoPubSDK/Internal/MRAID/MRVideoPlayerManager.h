//
//  MRVideoPlayerManager.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>

@protocol MRVideoPlayerManagerDelegate;

@interface MRVideoPlayerManager : NSObject

@property (nonatomic, weak) id<MRVideoPlayerManagerDelegate> delegate;

- (id)initWithDelegate:(id<MRVideoPlayerManagerDelegate>)delegate;
- (void)playVideo:(NSURL *)url;

@end

@protocol MRVideoPlayerManagerDelegate <NSObject>

- (UIViewController *)viewControllerForPresentingVideoPlayer;
- (void)videoPlayerManagerWillPresentVideo:(MRVideoPlayerManager *)manager;
- (void)videoPlayerManagerDidDismissVideo:(MRVideoPlayerManager *)manager;
- (void)videoPlayerManager:(MRVideoPlayerManager *)manager
        didFailToPlayVideoWithErrorMessage:(NSString *)message;

@end
