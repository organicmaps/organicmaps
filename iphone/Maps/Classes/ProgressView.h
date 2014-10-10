
#import <UIKit/UIKit.h>

@class ProgressView;
@protocol ProgressViewDelegate <NSObject>

- (void)progressViewDidCancel:(ProgressView *)progress;
- (void)progressViewDidStart:(ProgressView *)progress;

@end

@interface ProgressView : UIView

@property (nonatomic) BOOL failedMode;
@property (nonatomic) double progress;
- (void)setProgress:(double)progress animated:(BOOL)animated;

@property (nonatomic, weak) id <ProgressViewDelegate> delegate;

@end
