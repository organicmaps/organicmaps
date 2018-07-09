//
//  MPAdBrowserController.h
//  MoPub
//
//  Created by Nafis Jamal on 1/19/11.
//  Copyright 2011 MoPub, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPWebView.h"

#ifndef CF_RETURNS_RETAINED
#if __has_feature(attribute_cf_returns_retained)
#define CF_RETURNS_RETAINED __attribute__((cf_returns_retained))
#else
#define CF_RETURNS_RETAINED
#endif
#endif

@class MPAdConfiguration;

@protocol MPAdBrowserControllerDelegate;

@interface MPAdBrowserController : UIViewController <MPWebViewDelegate, UIActionSheetDelegate>

@property (nonatomic, strong) IBOutlet MPWebView *webView;
@property (nonatomic, strong) IBOutlet UIBarButtonItem *backButton;
@property (nonatomic, strong) IBOutlet UIBarButtonItem *forwardButton;
@property (nonatomic, strong) IBOutlet UIBarButtonItem *refreshButton;
@property (nonatomic, strong) IBOutlet UIBarButtonItem *safariButton;
@property (nonatomic, strong) IBOutlet UIBarButtonItem *doneButton;
@property (nonatomic, strong) IBOutlet UIBarButtonItem *spinnerItem;
@property (nonatomic, strong) UIActivityIndicatorView *spinner;

@property (nonatomic, weak) id<MPAdBrowserControllerDelegate> delegate;
@property (nonatomic, copy) NSURL *URL;

- (instancetype)initWithURL:(NSURL *)URL HTMLString:(NSString *)HTMLString delegate:(id<MPAdBrowserControllerDelegate>)delegate;
- (instancetype)initWithURL:(NSURL *)URL delegate:(id<MPAdBrowserControllerDelegate>)delegate;

// Navigation methods.
- (IBAction)back;
- (IBAction)forward;
- (IBAction)refresh;
- (IBAction)safari;
- (IBAction)done;

// Drawing methods.
- (CGContextRef)createContext CF_RETURNS_RETAINED;
- (UIImage *)backArrowImage;
- (UIImage *)forwardArrowImage;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@protocol MPAdBrowserControllerDelegate <NSObject>

- (void)dismissBrowserController:(MPAdBrowserController *)browserController animated:(BOOL)animated;

@optional

- (MPAdConfiguration *)adConfiguration;

@end
