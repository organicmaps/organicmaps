#import "MWMCuisineEditorTableViewCell.h"
#import "MWMCuisineEditorViewController.h"

namespace
{
  NSString * const kCuisineEditorCell = @"MWMCuisineEditorTableViewCell";
} // namespace

@interface MWMCuisineEditorViewController ()<UITableViewDelegate, UITableViewDataSource,
                                             MWMCuisineEditorTableViewCellProtocol>

@property (weak, nonatomic) IBOutlet UITableView * tableView;

@property (nonatomic) NSArray<NSString *> * cuisineKeys;

@property (nonatomic) NSMutableSet<NSString *> * selectedCuisines;

@end

@implementation MWMCuisineEditorViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self configNavBar];
  [self configData];
  [self configTable];
}

#pragma mark - Configuration

- (void)configNavBar
{
  self.title = L(@"edit_poi");
  self.navigationItem.leftBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
                                                    target:self
                                                    action:@selector(onCancel)];
  self.navigationItem.rightBarButtonItem =
      [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                                                    target:self
                                                    action:@selector(onDone)];
  self.navigationController.navigationBar.barStyle = UIBarStyleBlack;
}

- (void)configData
{
  NSString * stringsPath = [[NSBundle mainBundle] pathForResource:@"Localizable" ofType:@"strings"];
  NSArray<NSString *> * allKeys = [NSDictionary dictionaryWithContentsOfFile:stringsPath].allKeys;
  NSMutableSet<NSString *> * cuisineKeys = [NSMutableSet set];
  NSString * prefix = @"cuisine_";
  NSUInteger const prefixLength = prefix.length;
  for (NSString * key in allKeys)
  {
    if ([key hasPrefix:prefix])
      [cuisineKeys addObject:[key substringFromIndex:prefixLength]];
  }
  self.cuisineKeys = cuisineKeys.allObjects;
  self.selectedCuisines = [[self.delegate getCuisines] mutableCopy];
}

- (void)configTable
{
  [self.tableView registerNib:[UINib nibWithNibName:kCuisineEditorCell bundle:nil]
       forCellReuseIdentifier:kCuisineEditorCell];
}

#pragma mark - Actions

- (void)onCancel
{
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)onDone
{
  [self.delegate setCuisines:self.selectedCuisines];
  [self onCancel];
}

#pragma mark - MWMCuisineEditorTableViewCellProtocol

- (void)change:(NSString *)key selected:(BOOL)selected
{
  if (selected)
    [self.selectedCuisines addObject:key];
  else
    [self.selectedCuisines removeObject:key];
}

#pragma mark - UITableViewDataSource

- (UITableViewCell * _Nonnull)tableView:(UITableView * _Nonnull)tableView cellForRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  return [tableView dequeueReusableCellWithIdentifier:kCuisineEditorCell];
}

- (NSInteger)tableView:(UITableView * _Nonnull)tableView numberOfRowsInSection:(NSInteger)section
{
  return self.cuisineKeys.count;
}

#pragma mark - UITableViewDelegate

- (void)tableView:(UITableView * _Nonnull)tableView willDisplayCell:(MWMCuisineEditorTableViewCell * _Nonnull)cell forRowAtIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSInteger const index = indexPath.row;
  NSString * cuisine = self.cuisineKeys[index];
  BOOL const selected = [self.selectedCuisines containsObject:cuisine];
  [cell configWithDelegate:self key:cuisine selected:selected];
}

@end
