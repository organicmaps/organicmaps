
#import "LocationManager.h"
#import "ShareActionSheet.h"
#import <MessageUI/MFMessageComposeViewController.h>
#import <MessageUI/MFMailComposeViewController.h>
#import "Statistics.h"
#import "UIKitCategories.h"

#include "Framework.h"
#include "search/result.hpp"

@implementation ShareInfo

- (instancetype)initWithText:(NSString *)text lat:(double)lat lon:(double)lon myPosition:(BOOL)myPosition
{
  self = [super init];

  self.text = text ? text : @"";
  self.lat = lat;
  self.lon = lon;
  self.myPosition = myPosition;

  return self;
}

@end

@interface ShareActionSheet () <UIActionSheetDelegate, MFMailComposeViewControllerDelegate, MFMessageComposeViewControllerDelegate>

@property ShareInfo * info;
@property (weak) UIViewController * viewController;

@end

@implementation ShareActionSheet

- (instancetype)initWithInfo:(ShareInfo *)info viewController:(UIViewController *)viewController
{
  self = [super init];

  self.info = info;
  self.viewController = viewController;

  return self;
}

- (void)showFromRect:(CGRect)rect
{
  UIActionSheet * as = [[UIActionSheet alloc] initWithTitle:L(@"share") delegate:self cancelButtonTitle:nil destructiveButtonTitle:nil otherButtonTitles:nil];

  if ([MFMessageComposeViewController canSendText])
    [as addButtonWithTitle:L(@"message")];
  if ([MFMailComposeViewController canSendMail] || [self canUseGmailApp])
    [as addButtonWithTitle:L(@"email")];
  [as addButtonWithTitle:L(@"copy_link")];
  [as addButtonWithTitle:L(@"cancel")];
  [as setCancelButtonIndex:as.numberOfButtons - 1];
  [as showFromRect:rect inView:self.viewController.view animated:YES];
}

#define GE0_URL_LENGTH 16

- (void)actionSheet:(UIActionSheet *)as willDismissWithButtonIndex:(NSInteger)buttonIndex
{
  Framework & f = GetFramework();
  string const s = f.CodeGe0url(self.info.lat, self.info.lon, f.GetDrawScale(), [self.info.text UTF8String]);
  NSString * shortUrl = [NSString stringWithUTF8String:s.c_str()];

  if ([[as buttonTitleAtIndex:buttonIndex] isEqualToString:L(@"email")])
    [self sendEmailWithUrl:shortUrl];
  else if ([[as buttonTitleAtIndex:buttonIndex] isEqualToString:L(@"message")])
    [self sendMessageWithUrl:[shortUrl substringWithRange:NSMakeRange(0, GE0_URL_LENGTH)]];
  else if ([[as buttonTitleAtIndex:buttonIndex] isEqualToString:L(@"copy_link")])
    [UIPasteboard generalPasteboard].string = shortUrl;
}

- (BOOL)canUseGmailApp
{
  // This is not official google api
  return [[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString:@"googlegmail://"]];
}

- (void)sendEmailWithUrl:(NSString *)shortUrl
{
  NSString * subject;
  NSString * body;
  NSString * httpGe0Url = [shortUrl stringByReplacingCharactersInRange:NSMakeRange(0, 6) withString:@"http://ge0.me/"];
  if (self.info.myPosition)
  {
    search::AddressInfo info;
    GetFramework().GetAddressInfoForGlobalPoint(MercatorBounds::FromLatLon(self.info.lat, self.info.lon), info);
    NSString * nameAndAddress = [NSString stringWithUTF8String:info.FormatNameAndAddress().c_str()];
    body = [NSString stringWithFormat:L(@"my_position_share_email"), nameAndAddress, shortUrl, httpGe0Url];
    subject = L(@"my_position_share_email_subject");
  }
  else
  {
    body = [NSString stringWithFormat:L(@"bookmark_share_email"), self.info.text, shortUrl, httpGe0Url];
    subject = L(@"bookmark_share_email_subject");
  }

  if ([MFMailComposeViewController canSendMail])
  {
    MFMailComposeViewController * mailVC = [[MFMailComposeViewController alloc] init];
    [mailVC setMessageBody:body isHTML:NO];
    [mailVC setSubject:subject];
    mailVC.mailComposeDelegate = self;
    [self.viewController presentViewController:mailVC animated:YES completion:nil];
  }
  else if ([self canUseGmailApp])
  {
    subject = [subject stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    body = [body stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    NSString *urlString = [NSString stringWithFormat:@"googlegmail:///co?subject=%@&body=%@", subject, body];
    [[UIApplication sharedApplication] openURL:[NSURL URLWithString:urlString]];
  }
}

- (void)sendMessageWithUrl:(NSString *)shortUrl
{
  NSString * httpGe0Url = [shortUrl stringByReplacingCharactersInRange:NSMakeRange(0, 6) withString:@"http://ge0.me/"];
  MFMessageComposeViewController * messageVC = [[MFMessageComposeViewController alloc] init];
  if (self.info.myPosition)
    [messageVC setBody:[NSString stringWithFormat:L(@"my_position_share_sms"), shortUrl, httpGe0Url]];
  else
    [messageVC setBody:[NSString stringWithFormat:L(@"bookmark_share_sms"), shortUrl, httpGe0Url]];
  messageVC.messageComposeDelegate = self;
  [self.viewController presentViewController:messageVC animated:YES completion:nil];
}

- (void)messageComposeViewController:(MFMessageComposeViewController *)controller didFinishWithResult:(MessageComposeResult)result
{
  [self.viewController dismissViewControllerAnimated:YES completion:nil];
  if (result == MessageComposeResultSent)
    [[Statistics instance] logEvent:@"ge0(zero) MESSAGE Export"];
}

- (void)mailComposeController:(MFMailComposeViewController *)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError *)error
{
  [self.viewController dismissViewControllerAnimated:YES completion:nil];
  if (result == MFMailComposeResultSent || result == MFMailComposeResultSaved)
    [[Statistics instance] logEvent:@"ge0(zero) MAIL Export"];
}

@end
