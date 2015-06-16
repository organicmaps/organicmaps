//
//  MWMBookmarkDescriptionViewController.m
//  Maps
//
//  Created by v.mikhaylenko on 03.06.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

#import "MWMBookmarkDescriptionViewController.h"
#import "MWMPlacePageViewManager.h"
#import "MWMPlacePage.h"
#import "MWMPlacePageEntity.h"
#import "UIKitCategories.h"

static NSString * const kBookmarkDescriptionViewControllerNibName = @"MWMBookmarkDescriptionViewController";

typedef NS_ENUM(NSUInteger, BookmarkDescriptionState)
{
  BookmarkDescriptionStateEditText,
  BookmarkDescriptionStateViewHTML,
  BookmarkDescriptionStateEditHTML
};

@interface MWMBookmarkDescriptionViewController () <UIWebViewDelegate>

@property (weak, nonatomic) IBOutlet UITextView * textView;
@property (weak, nonatomic) IBOutlet UIWebView * webView;

@property (nonatomic) UIBarButtonItem * leftButton;
@property (nonatomic) UIBarButtonItem * rightButton;

@property (weak, nonatomic) MWMPlacePageViewManager * manager;
@property (nonatomic) BookmarkDescriptionState state;
@property (nonatomic) CGFloat realPlacePageHeight;

@end

@implementation MWMBookmarkDescriptionViewController


- (instancetype)initWithPlacePageManager:(MWMPlacePageViewManager *)manager
{
  self = [super initWithNibName:kBookmarkDescriptionViewControllerNibName bundle:nil];
  if (self)
    self.manager = manager;

  return self;
}

- (void)viewDidLoad
{
  [super viewDidLoad];
  [self.iPadOwnerNavigationController setNavigationBarHidden:NO];
  self.navigationItem.title = L(@"description");
  MWMPlacePageEntity const * entity = self.manager.entity;

  if (entity.isHTMLDescription)
    self.state = BookmarkDescriptionStateViewHTML;
  else
    self.state = BookmarkDescriptionStateEditText;
}

- (void)viewWillAppear:(BOOL)animated
{
  if (!self.iPadOwnerNavigationController)
    return;

  self.realPlacePageHeight = self.iPadOwnerNavigationController.view.height;
  CGFloat const bottomOffset = 88.;
  self.iPadOwnerNavigationController.view.height = self.textView.height + bottomOffset;
}

- (void)viewWillDisappear:(BOOL)animated
{
  [super viewWillDisappear:animated];
  if (!self.iPadOwnerNavigationController)
    return;

  [self.iPadOwnerNavigationController setNavigationBarHidden:YES];
  self.iPadOwnerNavigationController.view.height = self.realPlacePageHeight;
}

- (void)setState:(BookmarkDescriptionState)state
{
  MWMPlacePageEntity * entity = self.manager.entity;
  NSString * description = entity.bookmarkDescription;
  switch (state)
  {
    case BookmarkDescriptionStateEditText:
    case BookmarkDescriptionStateEditHTML:
      [self setupForEditingWithText:description];
      break;

    case BookmarkDescriptionStateViewHTML:
      [self setupForViewWithText:description];
      break;
  }
  _state = state;
}

- (void)setupForEditingWithText:(NSString *)text
{
  [UIView animateWithDuration:0.2f animations:^
  {
    self.webView.alpha = 0.;
    self.textView.alpha = 1.;
  }
  completion:^(BOOL finished)
  {
    self.textView.text = text;
  }];
  [self configureNavigationBarForEditing];
}

- (void)setupForViewWithText:(NSString *)text
{
  [UIView animateWithDuration:0.2f animations:^
  {
    self.webView.alpha = 1.;
    self.textView.alpha = 0.;
  }
  completion:^(BOOL finished)
  {
    [self.webView loadHTMLString:text baseURL:nil];
  }];
  [self configureNavigationBarForView];
}

- (void)configureNavigationBarForEditing
{
  self.leftButton = [[UIBarButtonItem alloc] initWithTitle:L(@"cancel") style:UIBarButtonItemStylePlain target:self action:@selector(cancelTap)];
  self.rightButton = [[UIBarButtonItem alloc] initWithTitle:L(@"done") style:UIBarButtonItemStylePlain target:self action:@selector(doneTap)];
  [self setupButtons];
}

- (void)configureNavigationBarForView
{
  self.leftButton = [[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"NavigationBarBackButton"] style:UIBarButtonItemStylePlain target:self action:@selector(backTap)];
  self.rightButton = [[UIBarButtonItem alloc] initWithTitle:L(@"edit") style:UIBarButtonItemStylePlain target:self action:@selector(editTap)];
  [self setupButtons];
}

- (void)setupButtons
{
  [self.navigationItem setLeftBarButtonItem:self.leftButton];
  [self.navigationItem setRightBarButtonItem:self.rightButton];
}

- (void)cancelTap
{
  if (self.manager.entity.isHTMLDescription)
    self.state = BookmarkDescriptionStateViewHTML;
  else
    [self.navigationController popViewControllerAnimated:YES];
}

- (void)doneTap
{
  MWMPlacePageEntity * entity = self.manager.entity;
  entity.bookmarkDescription = self.textView.text;
  [entity synchronize];
  [self.textView resignFirstResponder];

  if (entity.isHTMLDescription)
    self.state = BookmarkDescriptionStateViewHTML;
  else
    [self.navigationController popViewControllerAnimated:YES];
}

- (void)backTap
{
  [self.navigationController popViewControllerAnimated:YES];
}

- (void)editTap
{
  self.state = BookmarkDescriptionStateEditHTML;
}

- (BOOL)webView:(UIWebView *)inWeb shouldStartLoadWithRequest:(NSURLRequest *)inRequest navigationType:(UIWebViewNavigationType)inType
{
  if (inType == UIWebViewNavigationTypeLinkClicked)
  {
    [[UIApplication sharedApplication] openURL:[inRequest URL]];
    return NO;
  }
  return YES;
}

@end
