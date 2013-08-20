//
//  ShareActionSheet.m
//  Maps
//
//  Created by Kirill on 05/06/2013.
//  Copyright (c) 2013 MapsWithMe. All rights reserved.
//

#import "ShareActionSheet.h"
#import "Framework.h"
#import <MessageUI/MFMessageComposeViewController.h>
#import <MessageUI/MFMailComposeViewController.h>

#include "../../search/result.hpp"

#define GE0LENGTH 16

@implementation ShareActionSheet
+(void)showShareActionSheetInView:(id)view withObject:(id)del
{
  UIActionSheet * as = [[UIActionSheet alloc] initWithTitle:NSLocalizedString(@"share", nil) delegate:del cancelButtonTitle:nil  destructiveButtonTitle:nil otherButtonTitles:nil];
  if ([MFMessageComposeViewController canSendText])
    [as addButtonWithTitle:NSLocalizedString(@"message", nil)];
  if ([MFMailComposeViewController canSendMail])
    [as addButtonWithTitle:NSLocalizedString(@"email", nil)];
  [as addButtonWithTitle:NSLocalizedString(@"copy_link", nil)];
  [as addButtonWithTitle:NSLocalizedString(@"cancel", nil)];
  [as setCancelButtonIndex:as.numberOfButtons - 1];
  [as showInView:view];
  [as release];
}

+(void)resolveActionSheetChoice:(UIActionSheet *)as buttonIndex:(NSInteger)buttonIndex text:(NSString *)text
                           view:(id)view delegate:(id)del gX:(double)gX gY:(double)gY
                  andMyPosition:(BOOL)myPos
{
  NSString * shortUrl = [self generateShortUrlWithName:text gX:gX gY:gY];
  if ([[as buttonTitleAtIndex:buttonIndex] isEqualToString:NSLocalizedString(@"email", nil)])
    [self sendEmailWith:text andUrl:shortUrl view:view delegate:del gX:gX gY:gY myPosition:myPos];
  else if ([[as buttonTitleAtIndex:buttonIndex] isEqualToString:NSLocalizedString(@"message", nil)])
    [self sendMessageWithUrl:[shortUrl substringWithRange:NSMakeRange(0, GE0LENGTH)] view:view delegate:del myPosition:myPos];
  else if ([[as buttonTitleAtIndex:buttonIndex] isEqualToString:NSLocalizedString(@"copy_link", nil)])
    [UIPasteboard generalPasteboard].string = [self generateShortUrlWithName:text gX:gX gY:gY];
}

+(NSString *)generateShortUrlWithName:(NSString *)name gX:(double)gX gY:(double)gY
{
  Framework & f = GetFramework();
  string const s = f.CodeGe0url(MercatorBounds::YToLat(gY), MercatorBounds::XToLon(gX),
                                f.GetDrawScale(), [(name ? name : @"") UTF8String]);
  return [NSString stringWithUTF8String:s.c_str()];
}

+(void)sendEmailWith:(NSString *)textFieldText andUrl:(NSString *)shortUrl view:(id)view delegate:(id)del gX:(double)gX gY:(double)gY myPosition:(BOOL)myPos
{
  MFMailComposeViewController * mailVC = [[[MFMailComposeViewController alloc] init] autorelease];
  NSString * httpGe0Url = [shortUrl stringByReplacingCharactersInRange:NSMakeRange(0, 6) withString:@"http://ge0.me/"];
  if (myPos)
  {
    search::AddressInfo info;
    GetFramework().GetAddressInfoForGlobalPoint(m2::PointD(gX, gY), info);
    NSString * nameAndAddress = [NSString stringWithUTF8String:info.FormatNameAndAddress().c_str()];
    [mailVC setMessageBody:[NSString stringWithFormat:NSLocalizedString(@"my_position_share_email", nil), nameAndAddress, shortUrl, httpGe0Url] isHTML:NO];
    [mailVC setSubject:NSLocalizedString(@"my_position_share_email_subject", nil)];
  }
  else
  {
    [mailVC setMessageBody:[NSString stringWithFormat:NSLocalizedString(@"bookmark_share_email", nil), textFieldText, shortUrl, httpGe0Url] isHTML:NO];
    [mailVC setSubject:NSLocalizedString(@"bookmark_share_email_subject", nil)];
  }
  mailVC.mailComposeDelegate = del;
  [view presentModalViewController:mailVC animated:YES];
}

+(void)sendMessageWithUrl:(NSString *)shortUrl view:(id)view delegate:(id)del myPosition:(BOOL)myPos
{
  NSString * httpGe0Url = [shortUrl stringByReplacingCharactersInRange:NSMakeRange(0, 6) withString:@"http://ge0.me/"];
  MFMessageComposeViewController * messageVC = [[[MFMessageComposeViewController alloc] init] autorelease];

  if (myPos)
    [messageVC setBody:[NSString stringWithFormat:NSLocalizedString(@"my_position_share_sms", nil), shortUrl, httpGe0Url]];
  else
    [messageVC setBody:[NSString stringWithFormat:NSLocalizedString(@"bookmark_share_sms", nil), shortUrl, httpGe0Url]];
  messageVC.messageComposeDelegate = del;
  [view presentModalViewController:messageVC animated:YES];
}

@end
