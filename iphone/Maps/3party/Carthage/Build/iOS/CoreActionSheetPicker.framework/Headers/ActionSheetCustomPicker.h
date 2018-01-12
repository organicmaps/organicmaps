//
//  ActionSheetPicker.h
//  ActionSheetPicker
//
//  Created by  on 13/03/2012.
//  Copyright (c) 2012 Club 15CC. All rights reserved.
//
#import <UIKit/UIKit.h>
#import "AbstractActionSheetPicker.h"
#import "ActionSheetCustomPickerDelegate.h"

@interface ActionSheetCustomPicker : AbstractActionSheetPicker
{
}

/////////////////////////////////////////////////////////////////////////
#pragma mark - Properties
/////////////////////////////////////////////////////////////////////////
@property(nonatomic, strong) id <ActionSheetCustomPickerDelegate> delegate;


/////////////////////////////////////////////////////////////////////////
#pragma mark - Init Methods
/////////////////////////////////////////////////////////////////////////

/** Designated init */
- (instancetype)initWithTitle:(NSString *)title delegate:(id <ActionSheetCustomPickerDelegate>)delegate showCancelButton:(BOOL)showCancelButton origin:(id)origin;

- (instancetype)initWithTitle:(NSString *)title delegate:(id <ActionSheetCustomPickerDelegate>)delegate showCancelButton:(BOOL)showCancelButton origin:(id)origin initialSelections:(NSArray *)initialSelections;

/** Convenience class method for creating an launched */
+ (instancetype)showPickerWithTitle:(NSString *)title delegate:(id <ActionSheetCustomPickerDelegate>)delegate showCancelButton:(BOOL)showCancelButton origin:(id)origin;

+ (instancetype)showPickerWithTitle:(NSString *)title delegate:(id <ActionSheetCustomPickerDelegate>)delegate showCancelButton:(BOOL)showCancelButton origin:(id)origin initialSelections:(NSArray *)initialSelections;


@end
