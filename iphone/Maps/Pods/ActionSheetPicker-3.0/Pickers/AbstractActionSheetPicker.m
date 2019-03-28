//
//Copyright (c) 2011, Tim Cinel
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions are met:
//* Redistributions of source code must retain the above copyright
//notice, this list of conditions and the following disclaimer.
//* Redistributions in binary form must reproduce the above copyright
//notice, this list of conditions and the following disclaimer in the
//documentation and/or other materials provided with the distribution.
//* Neither the name of the <organization> nor the
//names of its contributors may be used to endorse or promote products
//derived from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
//DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#import "AbstractActionSheetPicker.h"
#import "SWActionSheet.h"
#import <objc/message.h>
#import <sys/utsname.h>

CG_INLINE BOOL isIPhone4() {
    struct utsname systemInfo;
    uname(&systemInfo);

    NSString *modelName = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
    return ([modelName rangeOfString:@"iPhone3"].location != NSNotFound);
}

#define IS_WIDESCREEN ( fabs( ( double )[ [ UIScreen mainScreen ] bounds ].size.height - ( double )568 ) < DBL_EPSILON )
#define IS_IPAD UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad
#define DEVICE_ORIENTATION [UIDevice currentDevice].orientation

// UIInterfaceOrientationMask vs. UIInterfaceOrientation
// As far as I know, a function like this isn't available in the API. I derived this from the enum def for
// UIInterfaceOrientationMask.
#define OrientationMaskSupportsOrientation(mask, orientation)   ((mask & (1 << orientation)) != 0)


#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 80000

@interface MyPopoverController : UIPopoverController <UIAdaptivePresentationControllerDelegate>
@end

@implementation MyPopoverController
+ (BOOL)canShowPopover {
    if (IS_IPAD) {
        if ([UITraitCollection class]) {
            UITraitCollection *traits = [UIApplication sharedApplication].keyWindow.traitCollection;
            if (traits.horizontalSizeClass == UIUserInterfaceSizeClassCompact)
                return NO;
        }
        return YES;
    }
    return NO;
}

- (UIModalPresentationStyle)adaptivePresentationStyleForPresentationController:(UIPresentationController *)controller traitCollection:(UITraitCollection *)traitCollection {
    return UIModalPresentationNone;
}
@end

#else

@interface MyPopoverController:UIPopoverController
@end

@implementation MyPopoverController
+(BOOL)canShowPopover {
    return IS_IPAD;
}
@end

#endif

@interface AbstractActionSheetPicker () <UIGestureRecognizerDelegate>

@property(nonatomic, strong) UIBarButtonItem *barButtonItem;
@property(nonatomic, strong) UIBarButtonItem *doneBarButtonItem;
@property(nonatomic, strong) UIBarButtonItem *cancelBarButtonItem;
@property(nonatomic, strong) UIView *containerView;
@property(nonatomic, unsafe_unretained) id target;
@property(nonatomic, assign) SEL successAction;
@property(nonatomic, assign) SEL cancelAction;
@property(nonatomic, strong) UIPopoverController *popOverController;
@property(nonatomic, strong) CIFilter *filter;
@property(nonatomic, strong) CIContext *context;
@property(nonatomic, strong) NSObject *selfReference;

- (void)presentPickerForView:(UIView *)aView;

- (void)configureAndPresentPopoverForView:(UIView *)aView;

- (void)configureAndPresentActionSheetForView:(UIView *)aView;

- (void)presentActionSheet:(SWActionSheet *)actionSheet;

- (void)presentPopover:(UIPopoverController *)popover;

- (void)dismissPicker;

- (BOOL)isViewPortrait;

- (BOOL)isValidOrigin:(id)origin;

- (id)storedOrigin;

- (UIToolbar *)createPickerToolbarWithTitle:(NSString *)aTitle;

- (UIBarButtonItem *)createButtonWithType:(UIBarButtonSystemItem)type target:(id)target action:(SEL)buttonAction;

- (IBAction)actionPickerDone:(id)sender;

- (IBAction)actionPickerCancel:(id)sender;
@end

@implementation AbstractActionSheetPicker

#pragma mark - Abstract Implementation

- (instancetype)init {
    self = [super init];
    if (self) {
        self.windowLevel = UIWindowLevelAlert;
        self.presentFromRect = CGRectZero;
        self.popoverBackgroundViewClass = nil;
        self.popoverDisabled = NO;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnavailableInDeploymentTarget"
        if ([UIApplication instancesRespondToSelector:@selector(supportedInterfaceOrientationsForWindow:)])
            self.supportedInterfaceOrientations = (UIInterfaceOrientationMask) [[UIApplication sharedApplication]
                    supportedInterfaceOrientationsForWindow:
                            [UIApplication sharedApplication].keyWindow];
        else {
            self.supportedInterfaceOrientations = UIInterfaceOrientationMaskAllButUpsideDown;
            if (IS_IPAD)
                self.supportedInterfaceOrientations |= (1 << UIInterfaceOrientationPortraitUpsideDown);
        }
#pragma clang diagnostic pop

        UIBarButtonItem *sysDoneButton = [self createButtonWithType:UIBarButtonSystemItemDone target:self
                                                             action:@selector(actionPickerDone:)];

        UIBarButtonItem *sysCancelButton = [self createButtonWithType:UIBarButtonSystemItemCancel target:self
                                                               action:@selector(actionPickerCancel:)];

        [self setCancelBarButtonItem:sysCancelButton];
        [self setDoneBarButtonItem:sysDoneButton];

        self.tapDismissAction = TapActionNone;
        //allows us to use this without needing to store a reference in calling class
        self.selfReference = self;

        NSMutableParagraphStyle *labelParagraphStyle = [[NSMutableParagraphStyle alloc] init];
        labelParagraphStyle.alignment = NSTextAlignmentCenter;
        self.pickerTextAttributes = [@{NSParagraphStyleAttributeName : labelParagraphStyle} mutableCopy];

        self.context = [CIContext contextWithOptions:nil];
        self.filter = [CIFilter filterWithName:@"CIGaussianBlur"];
    }

    return self;
}


- (void)setTextColor:(UIColor *)textColor {
    if (self.pickerTextAttributes) {
        self.pickerTextAttributes[NSForegroundColorAttributeName] = textColor;
    } else {
        self.pickerTextAttributes = [@{NSForegroundColorAttributeName : [UIColor whiteColor]} mutableCopy];
    }
}

- (instancetype)initWithTarget:(id)target successAction:(SEL)successAction cancelAction:(SEL)cancelActionOrNil origin:(id)origin {
    self = [self init];
    if (self) {
        self.target = target;
        self.successAction = successAction;
        self.cancelAction = cancelActionOrNil;

        if ([origin isKindOfClass:[UIBarButtonItem class]])
            self.barButtonItem = origin;
        else if ([origin isKindOfClass:[UIView class]])
            self.containerView = origin;
        else
            NSAssert(NO, @"Invalid origin provided to ActionSheetPicker ( %@ )", origin);
    }
    return self;
}

- (void)dealloc {
    //need to clear picker delegates and datasources, otherwise they may call this object after it's gone
    if ([self.pickerView respondsToSelector:@selector(setDelegate:)])
        [self.pickerView performSelector:@selector(setDelegate:) withObject:nil];

    if ([self.pickerView respondsToSelector:@selector(setDataSource:)])
        [self.pickerView performSelector:@selector(setDataSource:) withObject:nil];

    if ([self.pickerView respondsToSelector:@selector(removeTarget:action:forControlEvents:)])
        [((UIControl *) self.pickerView) removeTarget:nil action:NULL forControlEvents:UIControlEventAllEvents];

    self.target = nil;

    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (UIView *)configuredPickerView {
    NSAssert(NO, @"This is an abstract class, you must use a subclass of AbstractActionSheetPicker (like ActionSheetStringPicker)");
    return nil;
}

- (void)notifyTarget:(id)target didSucceedWithAction:(SEL)successAction origin:(id)origin {
    NSAssert(NO, @"This is an abstract class, you must use a subclass of AbstractActionSheetPicker (like ActionSheetStringPicker)");
}

- (void)notifyTarget:(id)target didCancelWithAction:(SEL)cancelAction origin:(id)origin {
    if (target && cancelAction && [target respondsToSelector:cancelAction]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
        [target performSelector:cancelAction withObject:origin];
#pragma clang diagnostic pop
    }
}

#pragma mark - Actions

- (void)showActionSheetPicker {
    UIView *masterView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, self.viewSize.width, 260)];

    // to fix bug, appeared only on iPhone 4 Device: https://github.com/skywinder/ActionSheetPicker-3.0/issues/5
    if (isIPhone4()) {
        masterView.backgroundColor = [UIColor colorWithRed:0.97 green:0.97 blue:0.97 alpha:1.0];
    }
    self.toolbar = [self createPickerToolbarWithTitle:self.title];
    [masterView addSubview:self.toolbar];

    //ios7 picker draws a darkened alpha-only region on the first and last 8 pixels horizontally, but blurs the rest of its background.  To make the whole popup appear to be edge-to-edge, we have to add blurring to the remaining left and right edges.
    if (NSFoundationVersionNumber > NSFoundationVersionNumber_iOS_6_1) {
        CGRect rect = CGRectMake(0, self.toolbar.frame.origin.y, _borderWidth, masterView.frame.size.height - self.toolbar.frame.origin.y);
        UIToolbar *leftEdge = [[UIToolbar alloc] initWithFrame:rect];
        rect.origin.x = masterView.frame.size.width - _borderWidth;
        UIToolbar *rightEdge = [[UIToolbar alloc] initWithFrame:rect];
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnavailableInDeploymentTarget"
        leftEdge.barTintColor = rightEdge.barTintColor = self.toolbar.barTintColor;
#pragma clang diagnostic pop
        [masterView insertSubview:leftEdge atIndex:0];
        [masterView insertSubview:rightEdge atIndex:0];
    }

    self.pickerView = [self configuredPickerView];
    NSAssert(_pickerView != NULL, @"Picker view failed to instantiate, perhaps you have invalid component data.");
    // toolbar hidden remove the toolbar frame and update pickerview frame
    if (self.toolbar.hidden) {
        int halfWidth = (int) (_borderWidth * 0.5f);
        masterView.frame = CGRectMake(0, 0, self.viewSize.width, 220);
        self.pickerView.frame = CGRectMake(0, halfWidth, self.viewSize.width, 220 - halfWidth);
    }
    [masterView addSubview:_pickerView];

    if ((![MyPopoverController canShowPopover] || self.popoverDisabled) && !self.pickerBackgroundColor && !self.toolbarBackgroundColor && [self.pickerBlurRadius intValue] > 0) {
        [self blurPickerBackground];
    } else {
        [self presentPickerForView:masterView];
    }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnavailableInDeploymentTarget"
    {
        switch (self.tapDismissAction) {
            case TapActionNone:
                break;
            case TapActionSuccess: {
                // add tap dismiss action
                self.actionSheet.window.userInteractionEnabled = YES;
                UITapGestureRecognizer *tapAction = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(actionPickerDone:)];
                tapAction.delegate = self;
                [self.actionSheet.window addGestureRecognizer:tapAction];
                break;
            }
            case TapActionCancel: {
                // add tap dismiss action
                self.actionSheet.window.userInteractionEnabled = YES;
                UITapGestureRecognizer *tapAction = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(actionPickerCancel:)];
                tapAction.delegate = self;
                [self.actionSheet.window addGestureRecognizer:tapAction];
                break;
            }
        };
    }
#pragma clang diagnostic pop

}

- (IBAction)actionPickerDone:(id)sender {
    [self notifyTarget:self.target didSucceedWithAction:self.successAction origin:[self storedOrigin]];
    [self dismissPicker];
}

- (IBAction)actionPickerCancel:(id)sender {
    [self notifyTarget:self.target didCancelWithAction:self.cancelAction origin:[self storedOrigin]];
    [self dismissPicker];
}

- (void)dismissPicker {
#if __IPHONE_4_1 <= __IPHONE_OS_VERSION_MAX_ALLOWED
    if (self.actionSheet)
#else
        if (self.actionSheet && [self.actionSheet isVisible])
#endif
        [_actionSheet dismissWithClickedButtonIndex:0 animated:YES];
    else if (self.popOverController && self.popOverController.popoverVisible)
        [_popOverController dismissPopoverAnimated:YES];
    self.actionSheet = nil;
    self.popOverController = nil;
    self.selfReference = nil;
}

#pragma mark - Custom Buttons

- (NSMutableArray *)customButtons {
    if (!_customButtons) {
        _customButtons = [[NSMutableArray alloc] init];
    }

    return _customButtons;
}

- (void)addCustomButtonWithTitle:(NSString *)title value:(id)value {
    if (!title)
        title = @"";
    if (!value)
        value = @0;
    NSDictionary *buttonDetails = @{
            kButtonTitle : title,
            kActionType : @(ActionTypeValue),
            kButtonValue : value
    };
    [self.customButtons addObject:buttonDetails];
}

- (void)addCustomButtonWithTitle:(NSString *)title actionBlock:(ActionBlock)block {
    if (!title)
        title = @"";
    if (!block)
        block = (^{
        });
    NSDictionary *buttonDetails = @{
            kButtonTitle : title,
            kActionType : @(ActionTypeBlock),
            kButtonValue : [block copy]
    };
    [self.customButtons addObject:buttonDetails];
}

- (void)addCustomButtonWithTitle:(NSString *)title target:(id)target selector:(SEL)selector {
    if (!title)
        title = @"";
    if (!target)
        target = [NSNull null];
    NSDictionary *buttonDetails = @{
            kButtonTitle : title,
            kActionType : @(ActionTypeSelector),
            kActionTarget : target,
            kButtonValue : [NSValue valueWithPointer:selector]
    };
    [self.customButtons addObject:buttonDetails];
}

- (IBAction)customButtonPressed:(id)sender {
    UIBarButtonItem *button = (UIBarButtonItem *) sender;
    NSInteger index = button.tag;
    NSAssert((index >= 0 && index < self.customButtons.count), @"Bad custom button tag: %ld, custom button count: %lu", (long) index, (unsigned long) self.customButtons.count);

    NSDictionary *buttonDetails = (self.customButtons)[(NSUInteger) index];
    NSAssert(buttonDetails != NULL, @"Custom button dictionary is invalid");

    ActionType actionType = (ActionType) [buttonDetails[kActionType] integerValue];
    switch (actionType) {
        case ActionTypeValue: {
            NSAssert([self.pickerView respondsToSelector:@
            selector(selectRow:inComponent:animated:)], @"customButtonPressed not overridden, cannot interact with subclassed pickerView");
            NSInteger buttonValue = [buttonDetails[kButtonValue] integerValue];
            UIPickerView *picker = (UIPickerView *) self.pickerView;
            NSAssert(picker != NULL, @"PickerView is invalid");
            [picker selectRow:buttonValue inComponent:0 animated:YES];
            if ([self respondsToSelector:@selector(pickerView:didSelectRow:inComponent:)]) {
                void (*objc_msgSendTyped)(id target, SEL _cmd, id pickerView, NSInteger row, NSInteger component) = (void *) objc_msgSend; // sending Integers as params
                objc_msgSendTyped(self, @selector(pickerView:didSelectRow:inComponent:), picker, buttonValue, 0);
            }
            break;
        }

        case ActionTypeBlock: {
            ActionBlock actionBlock = buttonDetails[kButtonValue];
            [self dismissPicker];
            if (actionBlock)
                actionBlock();
            break;
        }

        case ActionTypeSelector: {
            SEL selector = [buttonDetails[kButtonValue] pointerValue];
            id target = buttonDetails[kActionTarget];
            [self dismissPicker];
            if (target && [target respondsToSelector:selector]) {
                SuppressPerformSelectorLeakWarning (
                        [target performSelector:selector];
                );
            }
            break;
        }

        default:
            NSAssert(false, @"Unknown action type");
            break;
    }
}

// Allow the user to specify a custom cancel button
- (void)setCancelButton:(UIBarButtonItem *)button {
    if (!button) {
        self.hideCancel = YES;
        return;
    }

    if ([button.customView isKindOfClass:[UIButton class]]) {
        UIButton *uiButton = (UIButton *) button.customView;
        [uiButton addTarget:self action:@selector(actionPickerCancel:) forControlEvents:UIControlEventTouchUpInside];
    }
    else {
        [button setTarget:self];
        [button setAction:@selector(actionPickerCancel:)];
    }
    self.cancelBarButtonItem = button;
}

// Allow the user to specify a custom done button
- (void)setDoneButton:(UIBarButtonItem *)button {
    if ([button.customView isKindOfClass:[UIButton class]]) {
        UIButton *uiButton = (UIButton *) button.customView;
        [button setAction:@selector(actionPickerDone:)];
        [uiButton addTarget:self action:@selector(actionPickerDone:) forControlEvents:UIControlEventTouchUpInside];
    }
    else {
        [button setTarget:self];
        [button setAction:@selector(actionPickerDone:)];
    }
    [button setTarget:self];
    [button setAction:@selector(actionPickerDone:)];
    self.doneBarButtonItem = button;
}

- (void)hidePickerWithCancelAction {
    [self actionPickerCancel:nil];
}


- (UIToolbar *)createPickerToolbarWithTitle:(NSString *)title {
    CGRect frame = CGRectMake(0, 0, self.viewSize.width, 44);
    UIToolbar *pickerToolbar = [[UIToolbar alloc] initWithFrame:frame];
    pickerToolbar.barStyle = (NSFoundationVersionNumber > NSFoundationVersionNumber_iOS_6_1) ? UIBarStyleDefault : UIBarStyleBlackTranslucent;

    pickerToolbar.barTintColor = self.toolbarBackgroundColor;
    pickerToolbar.tintColor = self.toolbarButtonsColor;

    NSMutableArray *barItems = [[NSMutableArray alloc] init];

    if (!self.hideCancel) {
        [barItems addObject:self.cancelBarButtonItem];
    }

    NSInteger index = 0;
    for (NSDictionary *buttonDetails in self.customButtons) {
        NSString *buttonTitle = buttonDetails[kButtonTitle];

        UIBarButtonItem *button;
        if (NSFoundationVersionNumber > NSFoundationVersionNumber_iOS_6_1) {
            button = [[UIBarButtonItem alloc] initWithTitle:buttonTitle style:UIBarButtonItemStylePlain
                                                     target:self action:@selector(customButtonPressed:)];
        }
        else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            button = [[UIBarButtonItem alloc] initWithTitle:buttonTitle style:UIBarButtonItemStyleBordered
                                                     target:self action:@selector(customButtonPressed:)];
#pragma clang diagnostic pop
        }

        button.tag = index;
        [barItems addObject:button];
        index++;
    }

    UIBarButtonItem *flexSpace = [self createButtonWithType:UIBarButtonSystemItemFlexibleSpace target:nil action:nil];
    [barItems addObject:flexSpace];
    if (title) {
        UIBarButtonItem *labelButton;

        labelButton = [self createToolbarLabelWithTitle:title titleTextAttributes:self.titleTextAttributes andAttributedTitle:self.attributedTitle];

        [barItems addObject:labelButton];
        [barItems addObject:flexSpace];
    }
    [barItems addObject:self.doneBarButtonItem];

    [pickerToolbar setItems:barItems animated:NO];
    [pickerToolbar layoutIfNeeded];
    return pickerToolbar;
}

- (UIBarButtonItem *)createToolbarLabelWithTitle:(NSString *)aTitle
                             titleTextAttributes:(NSDictionary *)titleTextAttributes
                              andAttributedTitle:(NSAttributedString *)attributedTitle {
    UILabel *toolBarItemLabel = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 180, 30)];
    [toolBarItemLabel setTextAlignment:NSTextAlignmentCenter];
    [toolBarItemLabel setBackgroundColor:[UIColor clearColor]];

    CGFloat strikeWidth;
    CGSize textSize;


    if (titleTextAttributes) {
        toolBarItemLabel.attributedText = [[NSAttributedString alloc] initWithString:aTitle attributes:titleTextAttributes];
        textSize = toolBarItemLabel.attributedText.size;
    } else if (attributedTitle) {
        toolBarItemLabel.attributedText = attributedTitle;
        textSize = toolBarItemLabel.attributedText.size;
    }
    else {
        [toolBarItemLabel setTextColor:(NSFoundationVersionNumber > NSFoundationVersionNumber_iOS_6_1) ? [UIColor blackColor] : [UIColor whiteColor]];
        [toolBarItemLabel setFont:[UIFont boldSystemFontOfSize:16]];
        toolBarItemLabel.text = aTitle;

        if (NSFoundationVersionNumber > NSFoundationVersionNumber_iOS_6_1) {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnavailableInDeploymentTarget"
            textSize = [[toolBarItemLabel text] sizeWithAttributes:@{NSFontAttributeName : [toolBarItemLabel font]}];
#pragma clang diagnostic pop
        } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            textSize = [[toolBarItemLabel text] sizeWithFont:[toolBarItemLabel font]];
#pragma clang diagnostic pop
        }
    }

    strikeWidth = textSize.width;

    if (strikeWidth < 180) {
        [toolBarItemLabel sizeToFit];
    }

    UIBarButtonItem *buttonLabel = [[UIBarButtonItem alloc] initWithCustomView:toolBarItemLabel];
    return buttonLabel;
}

- (UIBarButtonItem *)createButtonWithType:(UIBarButtonSystemItem)type target:(id)target action:(SEL)buttonAction {

    UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:type target:target
                                                                               action:buttonAction];
    return barButton;
}

#pragma mark - Custom Color

- (void)setPickerBackgroundColor:(UIColor *)backgroundColor {
    _pickerBackgroundColor = backgroundColor;
    _actionSheet.bgView.backgroundColor = backgroundColor;
}

#pragma mark - Picker blur effect

- (void)blurPickerBackground {
    UIWindow *window = [UIApplication sharedApplication].delegate.window;
    UIViewController *rootViewController = window.rootViewController;

    UIView *masterView = self.pickerView.superview;

    self.pickerView.backgroundColor = [UIColor clearColor];
    masterView.backgroundColor = [UIColor clearColor];

    // Get the snapshot
    UIGraphicsBeginImageContext(rootViewController.view.bounds.size);
    [rootViewController.view drawViewHierarchyInRect:rootViewController.view.bounds afterScreenUpdates:NO];
    UIImage *backgroundImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();

    [self presentPickerForView:masterView];

    // Crop the snapshot to match picker frame
    CIImage *image = [CIImage imageWithCGImage:[backgroundImage CGImage]];
    [self.filter setValue:image forKey:kCIInputImageKey];
    [self.filter setValue:self.pickerBlurRadius forKey:kCIInputRadiusKey];

    CGRect blurFrame = [rootViewController.view convertRect:self.pickerView.frame fromView:masterView];
    // CoreImage coordinate system and UIKit coordinate system differs, so we need to adjust the frame
    blurFrame.origin.y = - (blurFrame.origin.y - rootViewController.view.frame.size.height) - blurFrame.size.height;

    CGImageRef imageRef = [self.context createCGImage:self.filter.outputImage fromRect:blurFrame];

    UIImageView *blurredImageView = [[UIImageView alloc] initWithFrame:self.pickerView.frame];
    blurredImageView.image = [UIImage imageWithCGImage:imageRef];
    blurredImageView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;

    [masterView addSubview:blurredImageView];
    [masterView sendSubviewToBack:blurredImageView];

    CGImageRelease(imageRef);
}

#pragma mark - Utilities and Accessors

- (CGSize)viewSize {
    if (IS_IPAD) {
        if (!self.popoverDisabled && [MyPopoverController canShowPopover])
            return CGSizeMake(320, 320);
        return [UIApplication sharedApplication].keyWindow.bounds.size;
    }

#if defined(__IPHONE_8_0)
    if (floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_7_1) {
        //iOS 7.1 or earlier
        if ([self isViewPortrait])
            return CGSizeMake(320, IS_WIDESCREEN ? 568 : 480);
        return CGSizeMake(IS_WIDESCREEN ? 568 : 480, 320);

    } else {
        //iOS 8 or later
        return [[UIScreen mainScreen] bounds].size;
    }
#else
    if ( [self isViewPortrait] )
        return CGSizeMake(320 , IS_WIDESCREEN ? 568 : 480);
    return CGSizeMake(IS_WIDESCREEN ? 568 : 480, 320);
#endif
}

- (BOOL)isViewPortrait {
    return UIInterfaceOrientationIsPortrait([UIApplication sharedApplication].statusBarOrientation);
}

- (BOOL)isValidOrigin:(id)origin {
    if (!origin)
        return NO;
    BOOL isButton = [origin isKindOfClass:[UIBarButtonItem class]];
    BOOL isView = [origin isKindOfClass:[UIView class]];
    return (isButton || isView);
}

- (id)storedOrigin {
    if (self.barButtonItem)
        return self.barButtonItem;
    return self.containerView;
}

#pragma mark - Popovers and ActionSheets

- (void)presentPickerForView:(UIView *)aView {
    self.presentFromRect = aView.frame;

    if (!self.popoverDisabled && [MyPopoverController canShowPopover])
        [self configureAndPresentPopoverForView:aView];
    else
        [self configureAndPresentActionSheetForView:aView];
}

- (void)configureAndPresentActionSheetForView:(UIView *)aView {
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(didRotate:) name:UIApplicationWillChangeStatusBarOrientationNotification object:nil];

    _actionSheet = [[SWActionSheet alloc] initWithView:aView windowLevel:self.windowLevel];
    if (self.pickerBackgroundColor) {
        _actionSheet.bgView.backgroundColor = self.pickerBackgroundColor;
    }

    [self presentActionSheet:_actionSheet];

    // Use beginAnimations for a smoother popup animation, otherwise the UIActionSheet pops into view
    [UIView beginAnimations:nil context:nil];
//    _actionSheet.bounds = CGRectMake(0, 0, self.viewSize.width, sheetHeight);
    [UIView commitAnimations];
}

- (void)didRotate:(NSNotification *)notification {
    if (OrientationMaskSupportsOrientation(self.supportedInterfaceOrientations, DEVICE_ORIENTATION))
        [self dismissPicker];
}

- (void)presentActionSheet:(SWActionSheet *)actionSheet {
    NSParameterAssert(actionSheet != NULL);
    if (self.barButtonItem)
        [actionSheet showFromBarButtonItem:_barButtonItem animated:YES];
    else
        [actionSheet showInContainerView];
}

- (void)configureAndPresentPopoverForView:(UIView *)aView {
    UIViewController *viewController = [[UIViewController alloc] initWithNibName:nil bundle:nil];
    viewController.view = aView;

    if (NSFoundationVersionNumber > NSFoundationVersionNumber_iOS_6_1) {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnavailableInDeploymentTarget"
        viewController.preferredContentSize = aView.frame.size;
#pragma clang diagnostic pop
    }
    else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        viewController.contentSizeForViewInPopover = viewController.view.frame.size;
#pragma clang diagnostic pop
    }

    _popOverController = [[MyPopoverController alloc] initWithContentViewController:viewController];
    _popOverController.delegate = self;
    if (self.pickerBackgroundColor) {
        self.popOverController.backgroundColor = self.pickerBackgroundColor;
    }
    if (self.popoverBackgroundViewClass) {
        [self.popOverController setPopoverBackgroundViewClass:self.popoverBackgroundViewClass];
    }

    [self presentPopover:_popOverController];
}

- (void)presentPopover:(UIPopoverController *)popover {
    NSParameterAssert(popover != NULL);
    if (self.barButtonItem) {
        if (_containerView != nil) {
            [popover presentPopoverFromRect:CGRectMake(_containerView.frame.size.width / 2.f, 0.f, 0, 0) inView:_containerView permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
        } else {
            [popover presentPopoverFromBarButtonItem:_barButtonItem permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
        }

        return;
    }
    else if ((self.containerView)) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [popover presentPopoverFromRect:_containerView.bounds inView:_containerView
                   permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];

        });
        return;
    }
    // Unfortunately, things go to hell whenever you try to present a popover from a table view cell.  These are failsafes.
    UIView *origin = nil;
    CGRect presentRect = CGRectZero;
    @try {
        origin = (_containerView.superview ? _containerView.superview : _containerView);
        presentRect = origin.bounds;
        dispatch_async(dispatch_get_main_queue(), ^{
            [popover presentPopoverFromRect:presentRect inView:origin
                   permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];

        });
    }
    @catch (NSException *exception) {
        origin = [[[[UIApplication sharedApplication] keyWindow] rootViewController] view];
        presentRect = CGRectMake(origin.center.x, origin.center.y, 1, 1);
        dispatch_async(dispatch_get_main_queue(), ^{
            [popover presentPopoverFromRect:presentRect inView:origin
                   permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];

        });
    }
}

#pragma mark - Popoverdelegate

- (void)popoverControllerDidDismissPopover:(UIPopoverController *)popoverController {
    switch (self.tapDismissAction) {
        case TapActionSuccess: {
            [self notifyTarget:self.target didSucceedWithAction:self.successAction origin:self.storedOrigin];
            break;
        }
        case TapActionNone:
        case TapActionCancel: {
            [self notifyTarget:self.target didCancelWithAction:self.cancelAction origin:self.storedOrigin];
            break;
        }
    };
}

#pragma mark UIGestureRecognizerDelegate

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer {
    CGPoint location = [gestureRecognizer locationInView:self.toolbar];
    return !CGRectContainsPoint(self.toolbar.bounds, location);
}

@end

