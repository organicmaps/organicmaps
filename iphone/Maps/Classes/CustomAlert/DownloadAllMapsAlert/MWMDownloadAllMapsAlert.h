//
//  MWMDownloadAllMapsAlert.h
//  Maps
//
//  Created by v.mikhaylenko on 10.03.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMAlert.h"

@interface MWMDownloadAllMapsAlert : MWMAlert

+ (instancetype)alert;

@property (nonatomic, weak, readonly) IBOutlet UIView *specsView;
@property (nonatomic, weak, readonly) IBOutlet UILabel *titleLabel;
@property (nonatomic, weak, readonly) IBOutlet UILabel *messageLabel;
@property (nonatomic, weak, readonly) IBOutlet UIButton *notNowButton;
@property (nonatomic, weak, readonly) IBOutlet UIButton *downloadButton;
@property (nonatomic, weak, readonly) IBOutlet UILabel *downloadMapsLabel;

@end
