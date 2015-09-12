#define IPAD (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)

#define L(str) NSLocalizedString(str, nil)

#define INTEGRAL(f) ([UIScreen mainScreen].scale == 1 ? floor(f) : f)
#define PIXEL 1.0 / [UIScreen mainScreen].scale
