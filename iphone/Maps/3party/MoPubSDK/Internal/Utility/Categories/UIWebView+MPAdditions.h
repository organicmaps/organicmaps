//
//  UIWebView+MPAdditions.h
//  MoPub
//
//  Created by Andrew He on 11/6/11.
//  Copyright (c) 2011 MoPub, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

extern NSString *const kJavaScriptDisableDialogSnippet;

@interface UIWebView (MPAdditions)

- (void)mp_setScrollable:(BOOL)scrollable;
- (void)disableJavaScriptDialogs;

@end
