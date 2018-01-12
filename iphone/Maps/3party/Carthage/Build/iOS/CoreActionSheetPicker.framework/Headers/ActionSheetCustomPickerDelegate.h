//
//  ActionSheetPickerDelegate.h
//  ActionSheetPicker
//
//  Created by  on 13/03/2012.
//  Copyright (c) 2012 Club 15CC. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AbstractActionSheetPicker.h"

@protocol ActionSheetCustomPickerDelegate <UIPickerViewDelegate, UIPickerViewDataSource>

@optional

/** 
 Allow the delegate to override default settings for the picker
 
 Allows for instance, ability to set separate delegates and data sources as well as GUI settings on the UIPickerView
 If not defined and explicily overridden then this class will be the delegate and dataSource.
 */
- (void)configurePickerView:(UIPickerView *)pickerView DEPRECATED_MSG_ATTRIBUTE("use -actionSheetPicker:configurePickerView: instead.");
- (void)actionSheetPicker:(AbstractActionSheetPicker *)actionSheetPicker configurePickerView:(UIPickerView *)pickerView;

/** 
 Success callback 
 
 \param actionSheetPicker   .pickerView property accesses the picker.  Requires a cast to UIView subclass for the picker
 \param origin              The entity which launched the ActionSheetPicker
 */
- (void)actionSheetPickerDidSucceed:(AbstractActionSheetPicker *)actionSheetPicker origin:(id)origin;

/** Cancel callback.  See actionSheetPickerDidSuccess:origin: */
- (void)actionSheetPickerDidCancel:(AbstractActionSheetPicker *)actionSheetPicker origin:(id)origin;


@required

@end