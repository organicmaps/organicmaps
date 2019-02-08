#import "MWMActivityIndicator.h"

@interface MWMActivityIndicator ()

@property(nonatomic, strong) UIImageView *spinner;

@end

@implementation MWMActivityIndicator

- (instancetype)init {
  return [self initWithFrame:CGRectMake(0, 0, 24.f, 24.f)];
}

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    [self setup];
  }

  return self;
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
  self = [super initWithCoder:aDecoder];
  if (self) {
    [self setup];
  }

  return self;
}

- (void)setup {
  _spinner = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"ic_24px_spinner"]];
  [self addSubview:_spinner];
  [NSLayoutConstraint activateConstraints:@[
                                            [_spinner.leftAnchor constraintEqualToAnchor:self.leftAnchor],
                                            [_spinner.topAnchor constraintEqualToAnchor:self.topAnchor],
                                            [_spinner.rightAnchor constraintEqualToAnchor:self.rightAnchor],
                                            [_spinner.bottomAnchor constraintEqualToAnchor:self.bottomAnchor]
                                            ]];
  CABasicAnimation *rotationAnimation = [CABasicAnimation animationWithKeyPath:@"transform.rotation.z"];
  rotationAnimation.toValue = @(M_PI * 2);
  rotationAnimation.duration = 1;
  rotationAnimation.cumulative = true;
  rotationAnimation.repeatCount = INT_MAX;

  [_spinner.layer addAnimation:rotationAnimation forKey:@"rotationAnimation"];
}

- (CGSize)intrinsicContentSize {
  return CGSizeMake(24.f, 24.f);
}

@end
