#import "InfoSection.h"

@interface InfoSection ()

@property(weak, nonatomic) id<InfoSectionDelegate> delegate;

@end

@implementation InfoSection

- (instancetype)initWithDelegate:(id<InfoSectionDelegate>)delegate {
  self = [super init];
  if (self) {
    _delegate = delegate;
  }
  return self;
}

- (NSInteger)numberOfRows {
  return 1;
}

- (NSString *)title {
  return L(@"placepage_place_description");
}

- (BOOL)canEdit {
  return NO;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRow:(NSInteger)row {
  return [self.delegate infoCellForTableView:tableView];
}

- (BOOL)didSelectRow:(NSInteger)row {
  return NO;
}

- (BOOL)deleteRow:(NSInteger)row {
  return YES;
}

@end
