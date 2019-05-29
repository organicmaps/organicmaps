#import "MWMDiscoveryGuideViewModel.h"

@interface MWMDiscoveryGuideViewModel()

@property(nonatomic, readwrite) NSString *title;
@property(nonatomic, readwrite) NSString *subtitle;
@property(nonatomic, readwrite) NSString *label;
@property(nonatomic, readwrite) NSString *imagePath;

@end

@implementation MWMDiscoveryGuideViewModel

- (instancetype)initWithTitle:(NSString *)title
                     subtitle:(NSString *)subtitle
                        label:(NSString *)label
                     imageURL:(NSString *) imagePath {
  self = [super init];
  if (self) {
    self.title = title;
    self.subtitle = subtitle;
    self.label = label;
    self.imagePath = imagePath;
  }
  return self;
}

@end
