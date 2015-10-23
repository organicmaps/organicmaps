
#import "RichTextVC.h"
#import "UIFont+MapsMeFonts.h"

@interface RichTextVC ()

@property (nonatomic) NSString * text;
@property (nonatomic) UITextView * textView;

@end

@implementation RichTextVC

- (instancetype)initWithText:(NSString *)text
{
  self = [super init];

  self.text = text;

  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];

  [self.view addSubview:self.textView];
  self.textView.text = self.text;
}

- (UITextView *)textView
{
  if (!_textView)
  {
    _textView = [[UITextView alloc] initWithFrame:self.view.bounds];
    _textView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    _textView.editable = NO;
    if ([_textView respondsToSelector:@selector(setTextContainerInset:)])
      _textView.textContainerInset = UIEdgeInsetsMake(10, 5, 10, 5);
    _textView.font = [UIFont regular16];
    _textView.dataDetectorTypes = UIDataDetectorTypeLink;
  }
  return _textView;
}

@end
