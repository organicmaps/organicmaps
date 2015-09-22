#import "BookmarksVC.h"
#import "MWMSearchBookmarksCell.h"
#import "UIFont+MapsMeFonts.h"

#include "Framework.h"

@interface MWMSearchBookmarksCell ()

@property (nonatomic) NSInteger index;
@property (nonatomic) BOOL isVisible;
@property (nonatomic) NSUInteger count;

@property (weak, nonatomic) IBOutlet UIButton * visibilityButton;
@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UILabel * countLabel;
@property (weak, nonatomic) IBOutlet UIImageView * openArrow;

@end

@implementation MWMSearchBookmarksCell

- (void)awakeFromNib
{
  self.layer.shouldRasterize = YES;
  self.layer.rasterizationScale = UIScreen.mainScreen.scale;
}

- (void)configForIndex:(NSInteger)index
{
  BookmarkCategory * cat = GetFramework().GetBmCategory(index);
  self.index = index;
  self.isVisible = cat->IsVisible();
  size_t userMarksCount = 0;
  {
    BookmarkCategory::Guard guard(*cat);
    userMarksCount = guard.m_controller.GetUserMarkCount();
  }
  self.count = userMarksCount + cat->GetTracksCount();
  self.titleLabel.text = @(cat->GetName().c_str());
}

- (IBAction)toggleVisibility
{
  self.isVisible = !self.isVisible;
  BookmarkCategory * cat = GetFramework().GetBmCategory(self.index);
  BookmarkCategory::Guard guard(*cat);
  guard.m_controller.SetIsVisible(self.isVisible);
  cat->SaveToKMLFile();
}

- (IBAction)openBookmarks
{
  BookmarksVC * bvc = [[BookmarksVC alloc] initWithCategory:self.index];
  UINavigationController * rootVC = (UINavigationController *)UIApplication.sharedApplication.delegate.window.rootViewController;
  [rootVC pushViewController:bvc animated:YES];
}

- (void)setTitle:(NSString *)title
{
  self.titleLabel.text = title;
}

+ (CGFloat)defaultCellHeight
{
  return 44.0;
}

- (CGFloat)cellHeight
{
  return ceil([self.contentView systemLayoutSizeFittingSize:UILayoutFittingCompressedSize].height);
}

#pragma mark - Properties

- (void)setIsVisible:(BOOL)isVisible
{
  _isVisible = self.visibilityButton.selected = isVisible;
}

- (void)setCount:(NSUInteger)count
{
  _count = count;
  self.countLabel.text = @(count).stringValue;
}

- (void)setIsLightTheme:(BOOL)isLightTheme
{
  _isLightTheme = isLightTheme;
  if (isLightTheme)
  {
    [self.visibilityButton setImage:[UIImage imageNamed:@"ic_hide_light"] forState:UIControlStateNormal];
    [self.visibilityButton setImage:[UIImage imageNamed:@"ic_show_light"] forState:UIControlStateSelected];
    self.openArrow.image = [UIImage imageNamed:@"ic_carrot_light"];
  }
  else
  {
    [self.visibilityButton setImage:[UIImage imageNamed:@"ic_hide_dark"] forState:UIControlStateNormal];
    [self.visibilityButton setImage:[UIImage imageNamed:@"ic_show_dark"] forState:UIControlStateSelected];
    self.openArrow.image = [UIImage imageNamed:@"ic_carrot_dark"];
  }
}

@end
