//
// Copyright (c) 2013 MoPub. All rights reserved.
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
