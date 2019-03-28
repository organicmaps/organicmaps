//
//  ActionSheetMultipleStringPicker.m
//  CoreActionSheetPicker
//
//  Created by Alejandro on 21/07/15.
//  Copyright (c) 2015 Petr Korolev. All rights reserved.
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
//åLOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#import "ActionSheetMultipleStringPicker.h"

@interface ActionSheetMultipleStringPicker()
@property (nonatomic,strong) NSArray *data; //Array of string arrays :)
@property (nonatomic,strong) NSArray *initialSelection;
@end

@implementation ActionSheetMultipleStringPicker

+ (instancetype)showPickerWithTitle:(NSString *)title rows:(NSArray *)strings initialSelection:(NSArray *)indexes doneBlock:(ActionMultipleStringDoneBlock)doneBlock cancelBlock:(ActionMultipleStringCancelBlock)cancelBlockOrNil origin:(id)origin {
    ActionSheetMultipleStringPicker * picker = [[ActionSheetMultipleStringPicker alloc] initWithTitle:title rows:strings initialSelection:indexes doneBlock:doneBlock cancelBlock:cancelBlockOrNil origin:origin];
    [picker showActionSheetPicker];
    return picker;
}

- (instancetype)initWithTitle:(NSString *)title rows:(NSArray *)strings initialSelection:(NSArray *)indexes doneBlock:(ActionMultipleStringDoneBlock)doneBlock cancelBlock:(ActionMultipleStringCancelBlock)cancelBlockOrNil origin:(id)origin {
    self = [self initWithTitle:title rows:strings initialSelection:indexes target:nil successAction:nil cancelAction:nil origin:origin];
    if (self) {
        self.onActionSheetDone = doneBlock;
        self.onActionSheetCancel = cancelBlockOrNil;
    }
    return self;
}

+ (instancetype)showPickerWithTitle:(NSString *)title rows:(NSArray *)data initialSelection:(NSArray *)indexes target:(id)target successAction:(SEL)successAction cancelAction:(SEL)cancelActionOrNil origin:(id)origin {
    ActionSheetMultipleStringPicker *picker = [[ActionSheetMultipleStringPicker alloc] initWithTitle:title rows:data initialSelection:indexes target:target successAction:successAction cancelAction:cancelActionOrNil origin:origin];
    [picker showActionSheetPicker];
    return picker;
}

- (instancetype)initWithTitle:(NSString *)title rows:(NSArray *)data initialSelection:(NSArray *)indexes target:(id)target successAction:(SEL)successAction cancelAction:(SEL)cancelActionOrNil origin:(id)origin {
    self = [self initWithTarget:target successAction:successAction cancelAction:cancelActionOrNil origin:origin];
    if (self) {
        self.data = data;
        self.initialSelection = indexes;
        self.title = title;
    }
    return self;
}


- (UIView *)configuredPickerView {
    if (!self.data)
        return nil;
    CGRect pickerFrame = CGRectMake(0, 40, self.viewSize.width, 216);
    UIPickerView *stringPicker = [[UIPickerView alloc] initWithFrame:pickerFrame];
    stringPicker.delegate = self;
    stringPicker.dataSource = self;

    [self performInitialSelectionInPickerView:stringPicker];

    if (self.data.count == 0) {
        stringPicker.showsSelectionIndicator = NO;
        stringPicker.userInteractionEnabled = NO;
    } else {
        stringPicker.showsSelectionIndicator = YES;
        stringPicker.userInteractionEnabled = YES;
    }

    //need to keep a reference to the picker so we can clear the DataSource / Delegate when dismissing
    self.pickerView = stringPicker;

    return stringPicker;
}

- (void)notifyTarget:(id)target didSucceedWithAction:(SEL)successAction origin:(id)origin {
    if (self.onActionSheetDone) {
        _onActionSheetDone(self, [self selectedIndexes], [self selection]);
        return;
    }
    else if (target && [target respondsToSelector:successAction]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
        [target performSelector:successAction withObject:self.selectedIndexes withObject:origin];
#pragma clang diagnostic pop
        return;
    }
    NSLog(@"Invalid target/action ( %s / %s ) combination used for ActionSheetPicker and done block is nil.", object_getClassName(target), sel_getName(successAction));
}

- (void)notifyTarget:(id)target didCancelWithAction:(SEL)cancelAction origin:(id)origin {
    if (self.onActionSheetCancel) {
        _onActionSheetCancel(self);
        return;
    }
    else if (target && cancelAction && [target respondsToSelector:cancelAction]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
        [target performSelector:cancelAction withObject:origin];
#pragma clang diagnostic pop
    }
}

#pragma mark - UIPickerViewDelegate / DataSource

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component {

}

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView {
    return [self.data count];
}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component {
    return ((NSArray *)self.data[component]).count;
}

- (NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component {
    id obj = (self.data)[(NSUInteger) row];

    // return the object if it is already a NSString,
    // otherwise, return the description, just like the toString() method in Java
    // else, return nil to prevent exception

    if ([obj isKindOfClass:[NSString class]])
        return obj;

    if ([obj respondsToSelector:@selector(description)])
        return [obj performSelector:@selector(description)];

    return nil;
}

- (NSAttributedString *)pickerView:(UIPickerView *)pickerView attributedTitleForRow:(NSInteger)row forComponent:(NSInteger)component {
    id obj = (self.data)[component][(NSUInteger) row];

    // return the object if it is already a NSString,
    // otherwise, return the description, just like the toString() method in Java
    // else, return nil to prevent exception

    if ([obj isKindOfClass:[NSString class]])
        return [[NSAttributedString alloc] initWithString:obj attributes:self.pickerTextAttributes];

    if ([obj respondsToSelector:@selector(description)])
        return [[NSAttributedString alloc] initWithString:[obj performSelector:@selector(description)] attributes:self.pickerTextAttributes];

    return nil;
}

- (UIView *)pickerView:(UIPickerView *)pickerView viewForRow:(NSInteger)row forComponent:(NSInteger)component reusingView:(UIView *)view {
    UILabel *pickerLabel = (UILabel *)view;
    if (pickerLabel == nil) {
        pickerLabel = [[UILabel alloc] init];
    }
    id obj = (self.data)[component][row];
    
    NSAttributedString *attributeTitle = nil;
    // use the object if it is already a NSString,
    // otherwise, use the description, just like the toString() method in Java
    // else, use String with no text to ensure this delegate do not return a nil value.
    
    if ([obj isKindOfClass:[NSString class]])
        attributeTitle = [[NSAttributedString alloc] initWithString:obj attributes:self.pickerTextAttributes];
    
    if ([obj respondsToSelector:@selector(description)])
        attributeTitle = [[NSAttributedString alloc] initWithString:[obj performSelector:@selector(description)] attributes:self.pickerTextAttributes];
    
    if (attributeTitle == nil) {
        attributeTitle = [[NSAttributedString alloc] initWithString:@"" attributes:self.pickerTextAttributes];
    }
    pickerLabel.attributedText = attributeTitle;
    return pickerLabel;
}


- (void)performInitialSelectionInPickerView:(UIPickerView *)pickerView {
    for (int i = 0; i < self.selectedIndexes.count; i++) {
        NSInteger row = [(NSNumber *)self.initialSelection[i] integerValue];
        [pickerView selectRow:row inComponent:i animated:NO];
    }
}

- (NSArray *)selection {
    NSMutableArray * array = [NSMutableArray array];
    for (int i = 0; i < self.data.count; i++) {
        id object = self.data[i][[(UIPickerView *)self.pickerView selectedRowInComponent:(NSInteger)i]];
        [array addObject: object];
    }
    return [array copy];
}

- (NSArray *)selectedIndexes {
    NSMutableArray * indexes = [NSMutableArray array];
    for (int i = 0; i < self.data.count; i++) {
        NSNumber *index = [NSNumber numberWithInteger:[(UIPickerView *)self.pickerView selectedRowInComponent:(NSInteger)i]];
        [indexes addObject: index];
    }
    return [indexes copy];
}

//- (CGFloat)pickerView:(UIPickerView *)pickerView widthForComponent:(NSInteger)component {
//    return pickerView.frame.size.width - 30;
//}

@end
