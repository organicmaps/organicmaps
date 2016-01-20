#import "MWMDownloadMapRequest.h"
#import "MWMSearchDownloadView.h"
#import "MWMSearchDownloadViewController.h"

@interface MWMSearchDownloadViewController () <MWMDownloadMapRequestDelegate>

@property (nonatomic) IBOutlet UIView * downloadRequestHolder;

@property (nonatomic) MWMDownloadMapRequest * downloadRequest;
@property (nonatomic) IBOutlet UIButton * dimButton;

@property (weak, nonatomic) id<MWMSearchDownloadProtocol> delegate;

@end

@implementation MWMSearchDownloadViewController

- (nonnull instancetype)initWithDelegate:(id<MWMSearchDownloadProtocol>)delegate
{
  self = [super init];
  if (self)
    self.delegate = delegate;
  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  self.downloadRequest =
      [[MWMDownloadMapRequest alloc] initWithParentView:self.downloadRequestHolder delegate:self];
}

- (void)refresh
{
  [self.view refresh];
}

- (void)viewDidAppear:(BOOL)animated
{
  [super viewDidAppear:animated];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillShow:)
                                               name:UIKeyboardWillShowNotification
                                             object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillhide:)
                                               name:UIKeyboardWillHideNotification
                                             object:nil];
}

- (void)viewDidDisappear:(BOOL)animated
{
  [super viewDidDisappear:animated];
  [[NSNotificationCenter defaultCenter] removeObserver:self];
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
  [self stateUpdated:MWMDownloadMapRequestStateDownload];
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
  MWMSearchDownloadViewState viewState = state == MWMDownloadMapRequestStateDownload
                                             ? MWMSearchDownloadViewStateProgress
                                             : MWMSearchDownloadViewStateRequest;
  ((MWMSearchDownloadView *)self.view).state = viewState;
}

- (void)selectMapsAction
{
  [self.delegate selectMapsAction];
}

@end
