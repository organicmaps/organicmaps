#import "MWMBookmarkColorViewController.h"
#import "Statistics.h"
#import "SwiftBridge.h"
#import "UIViewController+Navigation.h"

#include "std/array.hpp"

namespace
{
array<NSString *, 9> const kBookmarkColorsVariant
{{
  @"placemark-red",
  @"placemark-yellow",
  @"placemark-blue",
  @"placemark-green",
  @"placemark-purple",
  @"placemark-orange",
  @"placemark-brown",
  @"placemark-pink",
  @"placemark-hotel"
}};

} // namespace

@interface MWMBookmarkColorViewController ()

@property (copy, nonatomic) NSString * bookmarkColor;
@property (weak, nonatomic) id<MWMBookmarkColorDelegate> delegate;

@end

@implementation MWMBookmarkColorViewController

- (instancetype)initWithColor:(NSString *)color delegate:(id<MWMBookmarkColorDelegate>)delegate
{
  self = [super init];
  if (self)
  {
    _bookmarkColor = color;
    _delegate = delegate;
  }
  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  NSAssert(self.bookmarkColor, @"Color can't be nil!");
  NSAssert(self.delegate, @"Delegate can't be nil!");
  self.title = L(@"bookmark_color");
}

@end

@implementation MWMBookmarkColorViewController (TableView)

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto cell =
      [tableView dequeueReusableCellWithCellClass:[UITableViewCell class] indexPath:indexPath];
  NSString * currentColor = kBookmarkColorsVariant[indexPath.row];
  cell.textLabel.text = ios_bookmark_ui_helper::LocalizedTitleForBookmarkColor(currentColor);
  BOOL const isSelected = [currentColor isEqualToString:self.bookmarkColor];
  cell.imageView.image = ios_bookmark_ui_helper::ImageForBookmarkColor(currentColor, isSelected);
  cell.accessoryType = isSelected ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
  return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  return kBookmarkColorsVariant.size();
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
  NSString * bookmarkColor = kBookmarkColorsVariant[indexPath.row];
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatChangeBookmarkColor)
                   withParameters:@{kStatValue : bookmarkColor}];
  [self.delegate didSelectColor:bookmarkColor];
  [self backTap];
}

@end
