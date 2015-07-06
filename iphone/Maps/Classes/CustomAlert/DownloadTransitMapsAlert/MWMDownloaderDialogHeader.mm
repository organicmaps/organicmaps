//
//  MWMDownloaderDialogHeader.m
//  Maps
//
//  Created by v.mikhaylenko on 06.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMDownloaderDialogHeader.h"
#import "MWMDownloadTransitMapAlert.h"

static NSString * const kDownloaderDialogHeaderNibName = @"MWMDownloaderDialogHeader";

@interface MWMDownloaderDialogHeader ()

@property (weak, nonatomic) IBOutlet UILabel * title;
@property (weak, nonatomic) IBOutlet UILabel * size;
@property (weak, nonatomic) IBOutlet UIView * dividerView;
@property (weak, nonatomic) MWMDownloadTransitMapAlert * ownerAlert;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * sizeTrailing;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint * titleLeading;

@end

@implementation MWMDownloaderDialogHeader

+ (instancetype)headerForOwnerAlert:(MWMDownloadTransitMapAlert *)alert title:(NSString *)title size:(NSString *)size;
{
  MWMDownloaderDialogHeader * header = [[[NSBundle mainBundle] loadNibNamed:kDownloaderDialogHeaderNibName owner:nil options:nil] firstObject];
  header.title.text = title;
  header.size.text = size;
  header.ownerAlert = alert;
  return header;
}

- (IBAction)headerButtonTap:(UIButton *)sender
{
  BOOL const currentState = sender.selected;
  sender.selected = !currentState;
  self.dividerView.hidden = currentState;
  [UIView animateWithDuration:.15 animations:^
  {
    if (sender.selected)
      self.expandImage.transform = CGAffineTransformMakeRotation(M_PI);
    else
      self.expandImage.transform = CGAffineTransformIdentity;
  }];
  [self.ownerAlert showDownloadDetail:sender];
}

- (void)layoutSizeLabel
{
  if (self.expandImage.hidden)
    self.sizeTrailing.constant = self.titleLeading.constant;
  [self layoutIfNeeded];
}

@end
