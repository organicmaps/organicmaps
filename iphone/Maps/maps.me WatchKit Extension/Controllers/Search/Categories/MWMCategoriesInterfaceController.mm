//
//  MWMCategoriesInterfaceController.m
//  Maps
//
//  Created by v.mikhaylenko on 06.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMCategoriesInterfaceController.h"
#import "MWMWatchLocationTracker.h"
#import "Macros.h"
#import "MWMCategoriesInterfaceCell.h"

@interface MWMCategoriesInterfaceController()

@property (weak, nonatomic) IBOutlet WKInterfaceTable * table;
@property (nonatomic, readonly) NSArray * categories;

@end

static NSString * const kCategoriesCellIdentifier = @"CategoriesCell";
static NSString * const kSearchResultControllerIdentifier = @"SearchResultController";

@implementation MWMCategoriesInterfaceController

- (instancetype)init
{
  self = [super init];
  if (self)
    _categories = @[@"food", @"hotel", @"tourism", @"transport", @"fuel", @"shop", @"entertainment", @"atm", @"bank", @"wifi", @"parking", @"toilet", @"pharmacy", @"hospital", @"post", @"police"];
  return self;
}

- (void)awakeWithContext:(id)context
{
  [super awakeWithContext:context];
  [self setTitle:L(@"search")];
}

- (void)willActivate
{
  [super willActivate];
  if (self.haveLocation)
    [self loadCategories];
}

- (void)loadCategories
{
  dispatch_async(dispatch_get_main_queue(),
  ^{
    [self.table setNumberOfRows:self.categories.count withRowType:kCategoriesCellIdentifier];
    [self reloadTable];
  });
}

- (void)reloadTable
{
  [self.categories enumerateObjectsUsingBlock:^(NSString *str, NSUInteger idx, BOOL *stop)
  {
    MWMCategoriesInterfaceCell *cell = [self.table rowControllerAtIndex:idx];
    [cell.label setText:L(str)];
    [cell.icon setImageNamed:[NSString stringWithFormat:@"btn_%@", str]];
  }];
}

- (void)table:(WKInterfaceTable *)table didSelectRowAtIndex:(NSInteger)rowIndex
{
  NSString *query = self.categories[rowIndex];
  [self pushControllerWithName:kSearchResultControllerIdentifier context:query];
}

@end
