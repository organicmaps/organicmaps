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

@class ActionSheetDatePicker;

typedef void(^ActionDateDoneBlock)(ActionSheetDatePicker *picker, id selectedDate, id origin); //selectedDate is NSDate or NSNumber for "UIDatePickerModeCountDownTimer"
typedef void(^ActionDateCancelBlock)(ActionSheetDatePicker *picker);

@interface ActionSheetDatePicker : AbstractActionSheetPicker

@property (nonatomic, retain) NSDate *minimumDate; // specify min/max date range. default is nil. When min > max, the values are ignored. Ignored in countdown timer mode
@property (nonatomic, retain) NSDate *maximumDate; // default is nil

@property (nonatomic) NSInteger minuteInterval; // display minutes wheel with interval. interval must be evenly divided into 60. default is 1. min is 1, max is 30

@property (nonatomic, retain) NSLocale   *locale;   // default is [NSLocale currentLocale]. setting nil returns to default
@property (nonatomic, copy)   NSCalendar *calendar; // default is [NSCalendar currentCalendar]. setting nil returns to default
@property (nonatomic, retain) NSTimeZone *timeZone; // default is nil. use current time zone or time zone from calendar

@property (nonatomic, assign) NSTimeInterval countDownDuration; // for UIDatePickerModeCountDownTimer, ignored otherwise. default is 0.0. limit is 23:59 (86,399 seconds). value being set is div 60 (drops remaining seconds).

@property (nonatomic, copy) ActionDateDoneBlock onActionSheetDone;
@property (nonatomic, copy) ActionDateCancelBlock onActionSheetCancel;

+ (instancetype)showPickerWithTitle:(NSString *)title datePickerMode:(UIDatePickerMode)datePickerMode selectedDate:(NSDate *)selectedDate target:(id)target action:(SEL)action origin:(id)origin;

+ (instancetype)showPickerWithTitle:(NSString *)title datePickerMode:(UIDatePickerMode)datePickerMode selectedDate:(NSDate *)selectedDate target:(id)target action:(SEL)action origin:(id)origin cancelAction:(SEL)cancelAction;

+ (instancetype)showPickerWithTitle:(NSString *)title
           datePickerMode:(UIDatePickerMode)datePickerMode selectedDate:(NSDate *)selectedDate
              minimumDate:(NSDate *)minimumDate maximumDate:(NSDate *)maximumDate
                   target:(id)target action:(SEL)action origin:(id)origin;

+ (instancetype)showPickerWithTitle:(NSString *)title
           datePickerMode:(UIDatePickerMode)datePickerMode
             selectedDate:(NSDate *)selectedDate
                doneBlock:(ActionDateDoneBlock)doneBlock
              cancelBlock:(ActionDateCancelBlock)cancelBlock
                   origin:(UIView*)view;

+ (instancetype)showPickerWithTitle:(NSString *)title
           datePickerMode:(UIDatePickerMode)datePickerMode
             selectedDate:(NSDate *)selectedDate
              minimumDate:(NSDate *)minimumDate
              maximumDate:(NSDate *)maximumDate
                doneBlock:(ActionDateDoneBlock)doneBlock
              cancelBlock:(ActionDateCancelBlock)cancelBlock
                   origin:(UIView*)view;


- (id)initWithTitle:(NSString *)title datePickerMode:(UIDatePickerMode)datePickerMode selectedDate:(NSDate *)selectedDate target:(id)target action:(SEL)action origin:(id)origin;

- (instancetype)initWithTitle:(NSString *)title datePickerMode:(UIDatePickerMode)datePickerMode selectedDate:(NSDate *)selectedDate minimumDate:(NSDate *)minimumDate maximumDate:(NSDate *)maximumDate target:(id)target action:(SEL)action origin:(id)origin;

- (instancetype)initWithTitle:(NSString *)title datePickerMode:(UIDatePickerMode)datePickerMode selectedDate:(NSDate *)selectedDate target:(id)target action:(SEL)action origin:(id)origin cancelAction:(SEL)cancelAction;

- (instancetype)initWithTitle:(NSString *)title datePickerMode:(UIDatePickerMode)datePickerMode selectedDate:(NSDate *)selectedDate minimumDate:(NSDate *)minimumDate maximumDate:(NSDate *)maximumDate target:(id)target action:(SEL)action cancelAction:(SEL)cancelAction origin:(id)origin;


- (instancetype)initWithTitle:(NSString *)title
               datePickerMode:(UIDatePickerMode)datePickerMode
                 selectedDate:(NSDate *)selectedDate
                    doneBlock:(ActionDateDoneBlock)doneBlock
                  cancelBlock:(ActionDateCancelBlock)cancelBlock
                       origin:(UIView*)view;

- (void)eventForDatePicker:(id)sender;

@end
