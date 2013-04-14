#import "EditDescriptionVC.h"
#import "BalloonView.h"

@implementation EditDescriptionVC

- (id) initWithBalloonView:(BalloonView *)view
{
  self = [super init];
  if (self)
  {
    m_balloon = view;
    self.title = NSLocalizedString(@"description", @"EditDescription dialog title");
  }
  return self;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  return YES;
}

- (void)loadView
{
  UITextView * tv = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, 320, 480)];
  tv.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  tv.editable = YES;
  tv.dataDetectorTypes = UIDataDetectorTypeAll;
  // Get specific font for text editor from table cell
  UITableViewCell * tmpCell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:@"tmpCell"];
  tmpCell.detailTextLabel.text = @"tmpText";
  [tmpCell layoutSubviews];
  tv.textColor = tmpCell.detailTextLabel.textColor;
  tv.font = tmpCell.detailTextLabel.font;
  [tmpCell release];
  self.view = tv;
}

- (void)viewWillAppear:(BOOL)animated
{
  [self registerForKeyboardNotifications];
  UITextView * tv = (UITextView *)self.view;
  tv.text = m_balloon.description;
  [tv becomeFirstResponder];
}

- (void)viewWillDisappear:(BOOL)animated
{
  UITextView * tv = (UITextView *)self.view;
  m_balloon.description = tv.text.length ? tv.text : nil;
  [tv resignFirstResponder];
  [self unregisterKeyboardNotifications];
}

- (void)registerForKeyboardNotifications
{
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWasShown:)
                                               name:UIKeyboardDidShowNotification object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillBeHidden:)
                                               name:UIKeyboardWillHideNotification object:nil];
}

- (void)unregisterKeyboardNotifications
{
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)keyboardWasShown:(NSNotification*)aNotification
{
  NSDictionary * info = [aNotification userInfo];
  CGSize kbSize = [[info objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;

  // Fix keyboard size if orientation was changed
  // (strange that iOS doesn't do it itself...)
  if (self.interfaceOrientation == UIInterfaceOrientationLandscapeLeft
      || self.interfaceOrientation == UIInterfaceOrientationLandscapeRight)
    std::swap(kbSize.height, kbSize.width);

  UIEdgeInsets contentInsets = UIEdgeInsetsMake(0.0, 0.0, kbSize.height, 0.0);
  UITextView * tv = (UITextView *)self.view;
  tv.contentInset = contentInsets;
  tv.scrollIndicatorInsets = contentInsets;
}

- (void)keyboardWillBeHidden:(NSNotification*)aNotification
{
  UIEdgeInsets contentInsets = UIEdgeInsetsZero;
  UITextView * tv = (UITextView *)self.view;
  tv.contentInset = contentInsets;
  tv.scrollIndicatorInsets = contentInsets;
}

@end
