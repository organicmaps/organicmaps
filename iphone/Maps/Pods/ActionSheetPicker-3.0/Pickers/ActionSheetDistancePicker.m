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
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#import "ActionSheetDistancePicker.h"
#import <objc/message.h>

@interface ActionSheetDistancePicker()
@property (nonatomic, strong) NSString *bigUnitString;
@property (nonatomic, assign) NSInteger selectedBigUnit;
@property (nonatomic, assign) NSInteger bigUnitMax;
@property (nonatomic, assign) NSInteger bigUnitDigits;
@property (nonatomic, strong) NSString *smallUnitString;
@property (nonatomic, assign) NSInteger selectedSmallUnit;
@property (nonatomic, assign) NSInteger smallUnitMax;
@property (nonatomic, assign) NSInteger smallUnitDigits;
@end

@implementation ActionSheetDistancePicker

+ (instancetype)showPickerWithTitle:(NSString *)title bigUnitString:(NSString *)bigUnitString bigUnitMax:(NSInteger)bigUnitMax selectedBigUnit:(NSInteger)selectedBigUnit smallUnitString:(NSString *)smallUnitString smallUnitMax:(NSInteger)smallUnitMax selectedSmallUnit:(NSInteger)selectedSmallUnit target:(id)target action:(SEL)action origin:(id)origin {
    ActionSheetDistancePicker *picker = [[ActionSheetDistancePicker alloc] initWithTitle:title bigUnitString:bigUnitString bigUnitMax:bigUnitMax selectedBigUnit:selectedBigUnit smallUnitString:smallUnitString smallUnitMax:smallUnitMax selectedSmallUnit:selectedSmallUnit target:target action:action origin:origin];
    [picker showActionSheetPicker];
    return picker;
}

- (instancetype)initWithTitle:(NSString *)title bigUnitString:(NSString *)bigUnitString bigUnitMax:(NSInteger)bigUnitMax selectedBigUnit:(NSInteger)selectedBigUnit smallUnitString:(NSString *)smallUnitString smallUnitMax:(NSInteger)smallUnitMax selectedSmallUnit:(NSInteger)selectedSmallUnit target:(id)target action:(SEL)action origin:(id)origin {
    self = [super initWithTarget:target successAction:action cancelAction:nil origin:origin];
    if (self) {
        self.title = title;
        self.bigUnitString = bigUnitString;
        self.bigUnitMax = bigUnitMax;
        self.selectedBigUnit = selectedBigUnit;
        self.smallUnitString = smallUnitString;
        self.smallUnitMax = smallUnitMax;
        self.selectedSmallUnit = selectedSmallUnit;
        self.bigUnitDigits = [[NSString stringWithFormat:@"%li", (long)self.bigUnitMax] length];
        self.smallUnitDigits = [[NSString stringWithFormat:@"%li", (long)self.smallUnitMax] length];
    }
    return self;
}

+ (instancetype)showPickerWithTitle:(NSString *)title bigUnitString:(NSString *)bigUnitString bigUnitMax:(NSInteger)bigUnitMax selectedBigUnit:(NSInteger)selectedBigUnit smallUnitString:(NSString *)smallUnitString smallUnitMax:(NSInteger)smallUnitMax selectedSmallUnit:(NSInteger)selectedSmallUnit target:(id)target action:(SEL)action origin:(id)origin cancelAction:(SEL)cancelAction
{
   ActionSheetDistancePicker *picker = [[ActionSheetDistancePicker alloc] initWithTitle:title bigUnitString:bigUnitString bigUnitMax:bigUnitMax selectedBigUnit:selectedBigUnit smallUnitString:smallUnitString smallUnitMax:smallUnitMax selectedSmallUnit:selectedSmallUnit target:target action:action origin:origin cancelAction:cancelAction];
    [picker showActionSheetPicker];
    return picker;
}

- (instancetype)initWithTitle:(NSString *)title bigUnitString:(NSString *)bigUnitString bigUnitMax:(NSInteger)bigUnitMax selectedBigUnit:(NSInteger)selectedBigUnit smallUnitString:(NSString *)smallUnitString smallUnitMax:(NSInteger)smallUnitMax selectedSmallUnit:(NSInteger)selectedSmallUnit target:(id)target action:(SEL)action origin:(id)origin cancelAction:(SEL)cancelAction
{
    self = [super initWithTarget:target successAction:action cancelAction:cancelAction origin:origin];
    if (self) {
        self.title = title;
        self.bigUnitString = bigUnitString;
        self.bigUnitMax = bigUnitMax;
        self.selectedBigUnit = selectedBigUnit;
        self.smallUnitString = smallUnitString;
        self.smallUnitMax = smallUnitMax;
        self.selectedSmallUnit = selectedSmallUnit;
        self.bigUnitDigits = [[NSString stringWithFormat:@"%li", (long)self.bigUnitMax] length];
        self.smallUnitDigits = [[NSString stringWithFormat:@"%li", (long)self.smallUnitMax] length];
    }
    return self;
}


- (UIView *)configuredPickerView {
    CGRect distancePickerFrame = CGRectMake(0, 40, self.viewSize.width, 216);
    DistancePickerView *picker = [[DistancePickerView alloc] initWithFrame:distancePickerFrame];
    picker.delegate = self;
    picker.dataSource = self;
    picker.showsSelectionIndicator = YES;
//    [picker addLabel:self.bigUnitString forComponent:(NSUInteger) (self.bigUnitDigits - 1) forLongestString:nil];
//    [picker addLabel:self.smallUnitString forComponent:(NSUInteger) (self.bigUnitDigits + self.smallUnitDigits - 1)
//    forLongestString:nil];

    NSInteger unitSubtract = 0;
    NSInteger currentDigit = 0;

    for (int i = 0; i < self.bigUnitDigits; ++i) {
        NSInteger factor = (int)pow((double)10, (double)(self.bigUnitDigits - (i+1)));
        currentDigit = (( self.selectedBigUnit - unitSubtract ) / factor )  ;
        [picker selectRow:currentDigit inComponent:i animated:NO];
        unitSubtract += currentDigit * factor;
    }

    unitSubtract = 0;

    for (NSInteger i = self.bigUnitDigits + 1; i < self.bigUnitDigits + self.smallUnitDigits + 1; ++i) {
        NSInteger factor = (int)pow((double)10, (double)(self.bigUnitDigits + self.smallUnitDigits + 1 - (i+1)));
        currentDigit = (( self.selectedSmallUnit - unitSubtract ) / factor )  ;
        [picker selectRow:currentDigit inComponent:i animated:NO];
        unitSubtract += currentDigit * factor;
    }

    //need to keep a reference to the picker so we can clear the DataSource / Delegate when dismissing
    self.pickerView = picker;

    return picker;
}

- (void)notifyTarget:(id)target didSucceedWithAction:(SEL)action origin:(id)origin {
    NSInteger bigUnits = 0;
    NSInteger smallUnits = 0;
    DistancePickerView *picker = (DistancePickerView *)self.pickerView;
    for (int i = 0; i < self.bigUnitDigits; ++i)
        bigUnits += [picker selectedRowInComponent:i] * (int)pow((double)10, (double)(self.bigUnitDigits - (i + 1)));

    for (NSInteger i = self.bigUnitDigits + 1; i < self.bigUnitDigits + self.smallUnitDigits + 1; ++i)
        smallUnits += [picker selectedRowInComponent:i] * (int)pow((double)10, (double)((picker.numberOfComponents - i - 2)));

        //sending three objects, so can't use performSelector:
    if ([target respondsToSelector:action])
    {
        void (*response)(id, SEL, id, id,id) = (void (*)(id, SEL, id, id,id)) objc_msgSend;
        response(target, action, @(bigUnits), @(smallUnits), origin);
    }
    else
        NSAssert(NO, @"Invalid target/action ( %s / %s ) combination used for ActionSheetPicker", object_getClassName(target), sel_getName(action));
}

#pragma mark -
#pragma mark UIPickerViewDataSource

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView {
    return self.bigUnitDigits + self.smallUnitDigits + 2;
}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component {

    //for labels
    if (component == self.bigUnitDigits || component == self.bigUnitDigits + self.smallUnitDigits + 1)
        return 1;

    if (component + 1 <= self.bigUnitDigits) {
        if (component == 0)
            return self.bigUnitMax / (int)pow((double)10, (double)(self.bigUnitDigits - 1)) + 1;
        return 10;
    }
    if (component == self.bigUnitDigits + 1)
        return self.smallUnitMax / (int)pow((double)10, (double)(self.smallUnitDigits - 1)) + 1;
    return 10;
}

- (UIView *)pickerView:(UIPickerView *)pickerView viewForRow:(NSInteger)row forComponent:(NSInteger)component reusingView:(UIView *)view{

    CGFloat totalWidth = pickerView.frame.size.width - 30;
    CGFloat otherSize = (totalWidth )/(self.bigUnitDigits + self.smallUnitDigits + 2);

    UILabel  * label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, otherSize, 30)];

    label.textAlignment = NSTextAlignmentCenter;

    label.font = [UIFont boldSystemFontOfSize:20];

    if ( component == self.bigUnitDigits )
    {
        label.text = self.bigUnitString;
        return label;
    }
    else if ( component == self.bigUnitDigits + self.smallUnitDigits + 1 )
    {
        label.text = self.smallUnitString;
        return label;
    }

    label.font = [UIFont systemFontOfSize:20];
    label.text = [NSString stringWithFormat:@"%li", (long)row];
    return label;
}

- (CGFloat)pickerView:(UIPickerView *)pickerView widthForComponent:(NSInteger)component {
    CGFloat totalWidth = pickerView.frame.size.width - 30;
    CGFloat otherSize = (totalWidth )/(self.bigUnitDigits + self.smallUnitDigits + 2);
    return otherSize;
}


- (void)customButtonPressed:(id)sender {
    NSLog(@"Not implemented. If you get around to it, please contribute back to the project :)");
}

@end
