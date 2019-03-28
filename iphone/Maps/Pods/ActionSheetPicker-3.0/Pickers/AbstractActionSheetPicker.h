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

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@class SWActionSheet;

#define SuppressPerformSelectorLeakWarning(Stuff) \
do { \
_Pragma("clang diagnostic push") \
_Pragma("clang diagnostic ignored \"-Warc-performSelector-leaks\"") \
Stuff; \
_Pragma("clang diagnostic pop") \
} while (0)

typedef NS_ENUM(NSInteger, ActionType) {
    ActionTypeValue,
    ActionTypeSelector,
    ActionTypeBlock
};

typedef NS_ENUM(NSInteger, TapAction) {
    TapActionNone,
    TapActionSuccess,
    TapActionCancel
};

typedef void (^ActionBlock)(void);

static NSString *const kButtonValue = @"buttonValue";

static NSString *const kButtonTitle = @"buttonTitle";

static NSString *const kActionType = @"buttonAction";

static NSString *const kActionTarget = @"buttonActionTarget";

@interface AbstractActionSheetPicker : NSObject <UIPopoverControllerDelegate>
@property(nonatomic, strong) SWActionSheet *actionSheet;
@property (nonatomic) UIWindowLevel windowLevel;
@property(nonatomic, assign) NSInteger tag;
@property(nonatomic, assign) int borderWidth;
@property(nonatomic, strong) UIToolbar *toolbar;
@property(nonatomic, copy) NSString *title;
@property(nonatomic, strong) UIView *pickerView;
@property(nonatomic, readonly) CGSize viewSize;
@property(nonatomic, strong) NSMutableArray *customButtons;
@property(nonatomic, assign) BOOL hideCancel; // show or hide cancel button.
@property(nonatomic, assign) CGRect presentFromRect;
@property(nonatomic) NSDictionary *titleTextAttributes; // default is nil. Used to specify Title Label attributes.
@property(nonatomic) NSAttributedString *attributedTitle; // default is nil. If titleTextAttributes not nil this value ignored.
@property(nonatomic) NSMutableDictionary *pickerTextAttributes; // default with a NSMutableParagraphStyle to set label align center. Used to specify Picker Label attributes.
@property(nonatomic) UIColor *pickerBackgroundColor;
@property(nonatomic) UIColor *toolbarBackgroundColor;
@property(nonatomic, strong) UIColor *toolbarButtonsColor;
@property(nonatomic) NSNumber *pickerBlurRadius;
@property(nonatomic, retain) Class popoverBackgroundViewClass; //allow popover customization on iPad
@property(nonatomic) UIInterfaceOrientationMask supportedInterfaceOrientations; // You can set your own supportedInterfaceOrientations value to prevent dismissing picker in some special cases.
@property(nonatomic) TapAction tapDismissAction; // Specify, which action should be fired in case of tapping outside of the picker (on top darkened side). Default is TapActionNone.
@property(nonatomic) BOOL popoverDisabled; // Disable popover behavior on iPad


- (void)setTextColor:(UIColor *)textColor;

// For subclasses.
- (instancetype)initWithTarget:(id)target successAction:(SEL)successAction cancelAction:(SEL)cancelActionOrNil origin:(id)origin;

// Present the ActionSheetPicker
- (void)showActionSheetPicker;

// For subclasses.  This is used to send a message to the target upon a successful selection and dismissal of the picker (i.e. not canceled).
- (void)notifyTarget:(id)target didSucceedWithAction:(SEL)successAction origin:(id)origin;

// For subclasses.  This is an optional message upon cancelation of the picker.
- (void)notifyTarget:(id)target didCancelWithAction:(SEL)cancelAction origin:(id)origin;

// For subclasses.  This returns a configured picker view.  Subclasses should autorelease.
- (UIView *)configuredPickerView;

// Adds custom buttons to the left of the UIToolbar that select specified values
- (void)addCustomButtonWithTitle:(NSString *)title value:(id)value;

// Adds custom buttons to the left of the UIToolbar that implement specified block
- (void)addCustomButtonWithTitle:(NSString *)title actionBlock:(ActionBlock)block;

// Adds custom buttons to the left of the UIToolbar that implement specified selector
- (void)addCustomButtonWithTitle:(NSString *)title target:(id)target selector:(SEL)selector;

//For subclasses. This responds to a custom button being pressed.
- (IBAction)customButtonPressed:(id)sender;

// Allow the user to specify a custom cancel button
- (void)setCancelButton:(UIBarButtonItem *)button;

// Allow the user to specify a custom done button
- (void)setDoneButton:(UIBarButtonItem *)button;

// Hide picker programmatically
- (void)hidePickerWithCancelAction;

@end
