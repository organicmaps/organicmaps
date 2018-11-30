#import "MWMCategoryInfoCell.h"
#import "SwiftBridge.h"

#include "map/bookmark_helpers.hpp"

#include "kml/types.hpp"
#include "kml/type_utils.hpp"

@interface MWMCategoryInfoCell()

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UILabel * authorLabel;
@property (weak, nonatomic) IBOutlet UIButton * moreButton;
@property (weak, nonatomic) IBOutlet UITextView * infoTextView;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * infoToBottomConstraint;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * infoHeightConstraint;

@property (copy, nonatomic) NSAttributedString * info;
@property (copy, nonatomic) NSString * shortInfo;
@property (weak, nonatomic) id<MWMCategoryInfoCellDelegate> delegate;

@end

@implementation MWMCategoryInfoCell

- (void)awakeFromNib
{
  [super awakeFromNib];
  self.titleLabel.text = nil;
  self.authorLabel.text = nil;
  self.infoTextView.text = nil;
}

- (void)setExpanded:(BOOL)expanded
{
  _expanded = expanded;
  if (self.shortInfo.length > 0 && self.info.length > 0)
  {
    if (expanded)
      self.infoTextView.attributedText = self.info;
    else
      self.infoTextView.text = self.shortInfo;
  }

  self.infoToBottomConstraint.active = expanded;
  self.infoHeightConstraint.active = !expanded;
  self.moreButton.hidden = expanded;
}

- (void)updateWithCategoryData:(kml::CategoryData const &)data
                      delegate:(id<MWMCategoryInfoCellDelegate>)delegate
{
  self.delegate = delegate;
  self.titleLabel.text = @(GetPreferredBookmarkStr(data.m_name).c_str());
  self.authorLabel.text = [NSString stringWithCoreFormat:L(@"author_name_by_prefix")
                                               arguments:@[@(data.m_authorName.c_str())]];
  auto infoHtml = @(GetPreferredBookmarkStr(data.m_description).c_str());
  auto info = [NSAttributedString stringWithHtml:infoHtml
                               defaultAttributes: @{NSFontAttributeName : [UIFont regular14],
                                                    NSForegroundColorAttributeName: [UIColor blackPrimaryText]}];
  auto shortInfo = @(GetPreferredBookmarkStr(data.m_annotation).c_str());
  if (info.length > 0 && shortInfo.length > 0)
  {
    self.info = info;
    self.shortInfo = shortInfo;
    self.infoTextView.text = shortInfo;
  }
  else if (info.length > 0)
  {
    self.infoTextView.attributedText = info;
  }
  else
  {
    self.infoTextView.text = shortInfo;
    self.expanded = YES;
  }
}

- (void)prepareForReuse
{
  [super prepareForReuse];
  self.titleLabel.text = nil;
  self.authorLabel.text = nil;
  self.infoTextView.text = nil;
  self.expanded = NO;
  self.info = nil;
  self.shortInfo = nil;
  self.delegate = nil;
}

- (IBAction)onMoreButton:(UIButton *)sender
{
  [self.delegate categoryInfoCellDidPressMore:self];
}

@end
