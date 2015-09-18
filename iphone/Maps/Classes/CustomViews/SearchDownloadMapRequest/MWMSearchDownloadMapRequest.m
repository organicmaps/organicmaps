#import "MWMDownloadMapRequest.h"
#import "MWMSearchDownloadMapRequest.h"
#import "MWMSearchDownloadMapRequestView.h"

@interface MWMSearchDownloadMapRequest () <MWMDownloadMapRequestDelegate>

@property (nonatomic) IBOutlet MWMSearchDownloadMapRequestView * rootView;
@property (nonatomic) IBOutlet UIView * downloadRequestHolder;

@property (nonatomic) MWMDownloadMapRequest * downloadRequest;
@property (strong, nonatomic) IBOutlet UIButton * dimButton;

@property (weak, nonatomic) id <MWMSearchDownloadMapRequest> delegate;

@end

@implementation MWMSearchDownloadMapRequest

- (nonnull instancetype)initWithParentView:(nonnull UIView *)parentView delegate:(nonnull id <MWMSearchDownloadMapRequest>)delegate
{
  self = [super init];
  if (self)
  {
    [[NSBundle mainBundle] loadNibNamed:self.class.className owner:self options:nil];
    self.delegate = delegate;
    [parentView addSubview:self.rootView];
    self.downloadRequest = [[MWMDownloadMapRequest alloc] initWithParentView:self.downloadRequestHolder delegate:self];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillhide:) name:UIKeyboardWillHideNotification object:nil];
  }
  return self;
}

- (void)dealloc
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [self.rootView removeFromSuperview];
}

- (void)keyboardWillShow:(nonnull NSNotification *)aNotification
{
  UIButton * dim = self.dimButton;
  dim.hidden = NO;
  dim.alpha = 0.0;
  NSNumber * duration = aNotification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  [UIView animateWithDuration:duration.floatValue animations:^
  {
    dim.alpha = 1.0;
  }];
}

- (void)keyboardWillhide:(nonnull NSNotification *)aNotification
{
  UIButton * dim = self.dimButton;
  dim.alpha = 1.0;
  NSNumber * duration = aNotification.userInfo[UIKeyboardAnimationDurationUserInfoKey];
  [UIView animateWithDuration:duration.floatValue animations:^
  {
    dim.alpha = 0.0;
  }
  completion:^(BOOL finished)
  {
    dim.hidden = YES;
  }];
}

#pragma mark - Process control

- (void)downloadProgress:(CGFloat)progress countryName:(nonnull NSString *)countryName
{
  [self.downloadRequest downloadProgress:progress countryName:countryName];
}

- (void)setDownloadFailed
{
  [self.downloadRequest setDownloadFailed];
}

#pragma mark - Actions

- (IBAction)dimTouchUpInside:(nonnull UIButton *)sender
{
  [UIApplication.sharedApplication.keyWindow endEditing:YES];
}

#pragma mark - MWMDownloadMapRequestDelegate

- (void)stateUpdated:(enum MWMDownloadMapRequestState)state
{
  [self.rootView show:state == MWMDownloadMapRequestStateDownload ? MWMSearchDownloadMapRequestViewStateProgress : MWMSearchDownloadMapRequestViewStateRequest];
}

- (void)selectMapsAction
{
  [self.delegate selectMapsAction];
}

@end
