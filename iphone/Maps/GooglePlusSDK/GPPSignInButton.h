//
//  GPPSignInButton.h
//  Google+ iOS SDK
//
//  Copyright 2012 Google Inc.
//
//  Use of this SDK is subject to the Google+ Platform Terms of Service:
//  https://developers.google.com/+/terms
//

#import <UIKit/UIKit.h>

// The various layout styles supported by the GPPSignInButton.
// The minmum size of the button depends on the language used for text.
// The following dimensions (in points) fit for all languages:
// kGPPSignInButtonStyleStandard: 226 x 48
// kGPPSignInButtonStyleWide:     308 x 48
// kGPPSignInButtonStyleIconOnly:  46 x 48 (no text, fixed size)
typedef enum {
  kGPPSignInButtonStyleStandard = 0,
  kGPPSignInButtonStyleWide = 1,
  kGPPSignInButtonStyleIconOnly = 2
} GPPSignInButtonStyle;

// The various color schemes supported by the GPPSignInButton.
typedef enum {
  kGPPSignInButtonColorSchemeDark = 0,
  kGPPSignInButtonColorSchemeLight = 1
} GPPSignInButtonColorScheme;

// This class provides the Google+ sign-in button. You can instantiate this
// class programmatically or from a NIB file.  You should set up the
// |GPPSignIn| shared instance with your client ID and any additional scopes,
// implement the delegate methods for |GPPSignIn|, and add this button to your
// view hierarchy.
@interface GPPSignInButton : UIButton

// Sets the sign-in button layout style. The default style is standard.
- (void)setStyle:(GPPSignInButtonStyle)style;

// Sets the sign-in button color scheme. The default scheme is dark.
- (void)setColorScheme:(GPPSignInButtonColorScheme)colorScheme;

@end
