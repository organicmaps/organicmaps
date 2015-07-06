//
//  MWMDownloaderDialogHeader.h
//  Maps
//
//  Created by v.mikhaylenko on 06.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import <UIKit/UIKit.h>
@class MWMDownloadTransitMapAlert;

@interface MWMDownloaderDialogHeader : UIView

@property (weak, nonatomic) IBOutlet UIButton * headerButton;
@property (weak, nonatomic) IBOutlet UIImageView * expandImage;

+ (instancetype)headerForOwnerAlert:(MWMDownloadTransitMapAlert *)alert title:(NSString *)title size:(NSString *)size;
- (void)layoutSizeLabel;

@end
