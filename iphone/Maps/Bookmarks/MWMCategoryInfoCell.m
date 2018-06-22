#import "MWMCategoryInfoCell.h"

@interface MWMCategoryInfoCell()

@property (weak, nonatomic) IBOutlet UILabel * titleLabel;
@property (weak, nonatomic) IBOutlet UILabel * authorLabel;
@property (weak, nonatomic) IBOutlet UILabel * infoLabel;
@property (weak, nonatomic) IBOutlet UIButton * moreButton;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * infoToBottomConstraint;

@property (copy, nonatomic) NSString * info;
@property (copy, nonatomic) NSString * shortInfo;

@end

@implementation MWMCategoryInfoCell

- (void)awakeFromNib
{
  [super awakeFromNib];

  self.titleLabel.text = nil;
  self.authorLabel.text = nil;
  self.infoLabel.text = nil;
}

- (void)setExpanded:(BOOL)expanded
{
  _expanded = expanded;
  if (self.shortInfo.length > 0 && self.info.length > 0)
    self.infoLabel.text = expanded ? self.info : self.shortInfo;
  else
    self.infoLabel.numberOfLines = expanded ? 0 : 2;

  self.infoToBottomConstraint.active = expanded;
  self.moreButton.hidden = expanded;
}

- (void)updateWithTitle:(NSString *)title
                 author:(NSString *)author
                   info:(NSString *)info
               shortInfo:(NSString *)shortInfo
{
  self.titleLabel.text = title;
  self.authorLabel.text = author;
  if (info.length > 0 && shortInfo.length > 0)
  {
    self.info = info;
    self.shortInfo = shortInfo;
    self.infoLabel.text = shortInfo;
    self.infoLabel.numberOfLines = 0;
  }
  else if (info.length > 0 || shortInfo.length > 0)
  {
    self.infoLabel.text = info.length > 0 ? info : shortInfo;
  }
  else
  {
    self.expanded = YES;
  }
}

- (void)prepareForReuse
{
  [super prepareForReuse];

  self.titleLabel.text = nil;
  self.authorLabel.text = nil;
  self.infoLabel.text = nil;
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
