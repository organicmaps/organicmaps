#import "ShareActionSheet.h"
#import "Framework.h"
#import <MessageUI/MFMessageComposeViewController.h>
#import <MessageUI/MFMailComposeViewController.h>
#import "Statistics.h"

#include "../../search/result.hpp"

@implementation ShareInfo

- (instancetype)initWithText:(NSString *)text gX:(double)gX gY:(double)gY myPosition:(BOOL)myPosition
{
  self = [super init];

  self.text = text ? text : @"";
  self.gX = gX;
  self.gY = gY;
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

- (void)show
{
  UIActionSheet * as = [[UIActionSheet alloc] initWithTitle:NSLocalizedString(@"share", nil) delegate:self cancelButtonTitle:nil destructiveButtonTitle:nil otherButtonTitles:nil];

  if ([MFMessageComposeViewController canSendText])
    [as addButtonWithTitle:NSLocalizedString(@"message", nil)];
  if ([MFMailComposeViewController canSendMail] || [self canUseGmailApp])
    [as addButtonWithTitle:NSLocalizedString(@"email", nil)];
  [as addButtonWithTitle:NSLocalizedString(@"copy_link", nil)];
  [as addButtonWithTitle:NSLocalizedString(@"cancel", nil)];
  [as setCancelButtonIndex:as.numberOfButtons - 1];
  [as showInView:self.viewController.view];
}

#define GE0_URL_LENGTH 16

- (void)actionSheet:(UIActionSheet *)as willDismissWithButtonIndex:(NSInteger)buttonIndex
{
  Framework & f = GetFramework();
  string const s = f.CodeGe0url(MercatorBounds::YToLat(self.info.gY), MercatorBounds::XToLon(self.info.gX), f.GetDrawScale(), [self.info.text UTF8String]);
  NSString * shortUrl = [NSString stringWithUTF8String:s.c_str()];

  if ([[as buttonTitleAtIndex:buttonIndex] isEqualToString:NSLocalizedString(@"email", nil)])
    [self sendEmailWithUrl:shortUrl];
  else if ([[as buttonTitleAtIndex:buttonIndex] isEqualToString:NSLocalizedString(@"message", nil)])
    [self sendMessageWithUrl:[shortUrl substringWithRange:NSMakeRange(0, GE0_URL_LENGTH)]];
  else if ([[as buttonTitleAtIndex:buttonIndex] isEqualToString:NSLocalizedString(@"copy_link", nil)])
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
    GetFramework().GetAddressInfoForGlobalPoint(m2::PointD(self.info.gX, self.info.gY), info);
    NSString * nameAndAddress = [NSString stringWithUTF8String:info.FormatNameAndAddress().c_str()];
    body = [NSString stringWithFormat:NSLocalizedString(@"my_position_share_email", nil), nameAndAddress, shortUrl, httpGe0Url];
    subject = NSLocalizedString(@"my_position_share_email_subject", nil);
  }
  else
  {
    body = [NSString stringWithFormat:NSLocalizedString(@"bookmark_share_email", nil), self.info.text, shortUrl, httpGe0Url];
    subject = NSLocalizedString(@"bookmark_share_email_subject", nil);
  }

  if ([MFMailComposeViewController canSendMail])
  {
    MFMailComposeViewController * mailVC = [[MFMailComposeViewController alloc] init];
    [mailVC setMessageBody:body isHTML:NO];
    [mailVC setSubject:subject];
    mailVC.mailComposeDelegate = self;
    [self.viewController presentModalViewController:mailVC animated:YES];
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
    [messageVC setBody:[NSString stringWithFormat:NSLocalizedString(@"my_position_share_sms", nil), shortUrl, httpGe0Url]];
  else
    [messageVC setBody:[NSString stringWithFormat:NSLocalizedString(@"bookmark_share_sms", nil), shortUrl, httpGe0Url]];
  messageVC.messageComposeDelegate = self;
  [self.viewController presentModalViewController:messageVC animated:YES];
}

- (void)messageComposeViewController:(MFMessageComposeViewController *)controller didFinishWithResult:(MessageComposeResult)result
{
  [self.viewController dismissModalViewControllerAnimated:YES];
  if (result == MessageComposeResultSent)
    [[Statistics instance] logEvent:@"ge0(zero) MESSAGE Export"];
}

- (void)mailComposeController:(MFMailComposeViewController *)controller didFinishWithResult:(MFMailComposeResult)result error:(NSError *)error
{
  [self.viewController dismissModalViewControllerAnimated:YES];
  if (result == MFMailComposeResultSent || result == MFMailComposeResultSaved)
    [[Statistics instance] logEvent:@"ge0(zero) MAIL Export"];
}

@end
