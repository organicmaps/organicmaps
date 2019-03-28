//
//  ActionSheetPicker.m
//  ActionSheetPicker
//
//  Created by  on 13/03/2012.
//  Copyright (c) 2012 Club 15CC. All rights reserved.
//

#import "ActionSheetCustomPicker.h"

@interface ActionSheetCustomPicker ()
@property(nonatomic, strong) NSArray *initialSelections;
@end

@implementation ActionSheetCustomPicker

/////////////////////////////////////////////////////////////////////////
#pragma mark - Init
/////////////////////////////////////////////////////////////////////////

- (instancetype)initWithTitle:(NSString *)title delegate:(id <ActionSheetCustomPickerDelegate>)delegate showCancelButton:(BOOL)showCancelButton origin:(id)origin
{
    return [self initWithTitle:title delegate:delegate
              showCancelButton:showCancelButton origin:origin
             initialSelections:nil];
}

+ (instancetype)showPickerWithTitle:(NSString *)title delegate:(id <ActionSheetCustomPickerDelegate>)delegate showCancelButton:(BOOL)showCancelButton origin:(id)origin
{
    return [self showPickerWithTitle:title delegate:delegate showCancelButton:showCancelButton origin:origin
                   initialSelections:nil ];
}

- (instancetype)initWithTitle:(NSString *)title delegate:(id <ActionSheetCustomPickerDelegate>)delegate showCancelButton:(BOOL)showCancelButton origin:(id)origin initialSelections:(NSArray *)initialSelections
{
    if ( self = [self initWithTarget:nil successAction:nil cancelAction:nil origin:origin] )
    {

        self.title = title;
        self.hideCancel = !showCancelButton;
        NSAssert(delegate, @"Delegate can't be nil");
        _delegate = delegate;
        if (initialSelections)
            self.initialSelections = [[NSArray alloc] initWithArray:initialSelections];
    }
    return self;
}

/////////////////////////////////////////////////////////////////////////

+ (instancetype)showPickerWithTitle:(NSString *)title delegate:(id <ActionSheetCustomPickerDelegate>)delegate showCancelButton:(BOOL)showCancelButton origin:(id)origin initialSelections:(NSArray *)initialSelections
{
    ActionSheetCustomPicker *picker = [[ActionSheetCustomPicker alloc] initWithTitle:title delegate:delegate
                                                                    showCancelButton:showCancelButton origin:origin
                                                                   initialSelections:initialSelections];
    [picker showActionSheetPicker];
    return picker;
}

/////////////////////////////////////////////////////////////////////////
#pragma mark - AbstractActionSheetPicker fulfilment
/////////////////////////////////////////////////////////////////////////

- (UIView *)configuredPickerView
{
    CGRect pickerFrame = CGRectMake(0, 40, self.viewSize.width, 216);
    UIPickerView *pv = [[UIPickerView alloc] initWithFrame:pickerFrame];
    self.pickerView = pv;

    // Default to our delegate being the picker's delegate and datasource
    pv.delegate = _delegate;
    pv.dataSource = _delegate;
    pv.showsSelectionIndicator = YES;

    if ( self.initialSelections )
    {
        NSAssert(pv.numberOfComponents == self.initialSelections.count, @"Number of sections not match");
        for (NSUInteger i = 0; i < [self.initialSelections count]; i++)
        {

            NSInteger row = [(NSNumber *) self.initialSelections[i] integerValue];
            NSAssert([pv numberOfRowsInComponent:i] > row, @"Number of sections not match");
            [pv selectRow:row inComponent:i animated:NO];

            // Strangely, the above selectRow:inComponent:animated: will not call
            // pickerView:didSelectRow:inComponent: automatically, so we manually call it.
            [pv reloadAllComponents];
        }

    }

    // Allow the delegate to override and set additional configs
    //to backward compatibility:
    if ( [_delegate respondsToSelector:@selector(actionSheetPicker:configurePickerView:)] )
    {
        [_delegate actionSheetPicker:self configurePickerView:pv];
    }
    return pv;
}


/////////////////////////////////////////////////////////////////////////

- (void)notifyTarget:(id)target didSucceedWithAction:(SEL)successAction origin:(id)origin
{
    // Ignore parent args and just notify the delegate
    if ( [_delegate respondsToSelector:@selector(actionSheetPickerDidSucceed:origin:)] )
    {
        [_delegate actionSheetPickerDidSucceed:self origin:origin];
    }
}

/////////////////////////////////////////////////////////////////////////

- (void)notifyTarget:(id)target didCancelWithAction:(SEL)cancelAction origin:(id)origin
{
    // Ignore parent args and just notify the delegate
    if ( [_delegate respondsToSelector:@selector(actionSheetPickerDidCancel:origin:)] )
    {
        [_delegate actionSheetPickerDidCancel:self origin:origin];
    }
}


@end
