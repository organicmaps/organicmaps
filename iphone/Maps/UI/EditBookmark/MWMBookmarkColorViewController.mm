#import "MWMBookmarkColorViewController.h"
#import "Statistics.h"
#import "SwiftBridge.h"

namespace
{
std::array<kml::PredefinedColor, 16> const kBookmarkColorsVariant
{{
  kml::PredefinedColor::Red,
  kml::PredefinedColor::Pink,
  kml::PredefinedColor::Purple,
  kml::PredefinedColor::DeepPurple,
  kml::PredefinedColor::Blue,
  kml::PredefinedColor::LightBlue,
  kml::PredefinedColor::Cyan,
  kml::PredefinedColor::Teal,
  kml::PredefinedColor::Green,
  kml::PredefinedColor::Lime,
  kml::PredefinedColor::Yellow,
  kml::PredefinedColor::Orange,
  kml::PredefinedColor::DeepOrange,
  kml::PredefinedColor::Brown,
  kml::PredefinedColor::Gray,
  kml::PredefinedColor::BlueGray
}};

} // namespace

@interface MWMBookmarkColorViewController ()

@property (nonatomic) kml::PredefinedColor bookmarkColor;
@property (weak, nonatomic) id<MWMBookmarkColorDelegate> delegate;

@end

@implementation MWMBookmarkColorViewController

- (instancetype)initWithColor:(kml::PredefinedColor)color delegate:(id<MWMBookmarkColorDelegate>)delegate
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
  NSAssert(self.bookmarkColor != kml::PredefinedColor::None, @"Color can't be None!");
  NSAssert(self.delegate, @"Delegate can't be nil!");
  self.title = L(@"bookmark_color");
}

@end

@implementation MWMBookmarkColorViewController (TableView)

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  auto cell =
      [tableView dequeueReusableCellWithCellClass:[UITableViewCell class] indexPath:indexPath];
  auto currentColor = kBookmarkColorsVariant[indexPath.row];
  cell.textLabel.text = ios_bookmark_ui_helper::LocalizedTitleForBookmarkColor(currentColor);
  BOOL const isSelected = currentColor == self.bookmarkColor;
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
  auto bookmarkColor = kBookmarkColorsVariant[indexPath.row];
  [Statistics logEvent:kStatEventName(kStatPlacePage, kStatChangeBookmarkColor)
                   withParameters:@{kStatValue : ios_bookmark_ui_helper::LocalizedTitleForBookmarkColor(bookmarkColor)}];
  [self.delegate didSelectColor:bookmarkColor];
  [self goBack];
}

@end
