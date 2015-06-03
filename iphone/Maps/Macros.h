//
//  Macros.h
//  Maps
//
//  Created by v.mikhaylenko on 07.04.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#define IPAD (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)

#define L(str) NSLocalizedString(str, nil)

#define INTEGRAL(f) ([UIScreen mainScreen].scale == 1 ? floor(f) : f)
#define PIXEL 1.0 / [UIScreen mainScreen].scale
