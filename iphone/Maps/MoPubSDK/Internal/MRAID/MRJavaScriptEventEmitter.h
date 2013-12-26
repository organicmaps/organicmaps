//
//  MRJavaScriptEventEmitter.h
//  MoPub
//
//  Copyright (c) 2013 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@class MRProperty;

@interface MRJavaScriptEventEmitter : NSObject

- (id)initWithWebView:(UIWebView *)webView;
- (NSString *)executeJavascript:(NSString *)javascript, ...;
- (void)fireChangeEventForProperty:(MRProperty *)property;
- (void)fireChangeEventsForProperties:(NSArray *)properties;
- (void)fireErrorEventForAction:(NSString *)action withMessage:(NSString *)message;
- (void)fireReadyEvent;
- (void)fireNativeCommandCompleteEvent:(NSString *)command;

@end
