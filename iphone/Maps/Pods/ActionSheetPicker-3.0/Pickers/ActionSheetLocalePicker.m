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
//åLOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#import "ActionSheetLocalePicker.h"

@interface ActionSheetLocalePicker()
//@property (nonatomic,strong) NSArray *data;
//@property (nonatomic,assign) NSInteger selectedIndex;

@property(nonatomic, strong) NSTimeZone *initialTimeZone;

@property (nonatomic, strong) NSString *selectedContinent;
@property (nonatomic, strong) NSString *selectedCity;

@property(nonatomic, strong) NSMutableDictionary *continentsAndCityDictionary;
@property(nonatomic, strong) NSMutableArray *continents;
@end

@implementation ActionSheetLocalePicker

+ (instancetype)showPickerWithTitle:(NSString *)title initialSelection:(NSTimeZone *)index doneBlock:(ActionLocaleDoneBlock)doneBlock cancelBlock:(ActionLocaleCancelBlock)cancelBlockOrNil origin:(id)origin
{
    ActionSheetLocalePicker * picker = [[ActionSheetLocalePicker alloc] initWithTitle:title initialSelection:index doneBlock:doneBlock cancelBlock:cancelBlockOrNil origin:origin];
    [picker showActionSheetPicker];
    return picker;
}

- (instancetype)initWithTitle:(NSString *)title initialSelection:(NSTimeZone *)timeZone doneBlock:(ActionLocaleDoneBlock)doneBlock cancelBlock:(ActionLocaleCancelBlock)cancelBlockOrNil origin:(id)origin
{
    self = [self initWithTitle:title initialSelection:timeZone target:nil successAction:nil cancelAction:nil origin:origin];
    if (self) {
        self.onActionSheetDone = doneBlock;
        self.onActionSheetCancel = cancelBlockOrNil;
    }
    return self;
}

+ (instancetype)showPickerWithTitle:(NSString *)title initialSelection:(NSTimeZone *)index target:(id)target successAction:(SEL)successAction cancelAction:(SEL)cancelActionOrNil origin:(id)origin
{
    ActionSheetLocalePicker *picker = [[ActionSheetLocalePicker alloc] initWithTitle:title initialSelection:index target:target successAction:successAction cancelAction:cancelActionOrNil origin:origin];
    [picker showActionSheetPicker];
    return picker;
}

- (instancetype)initWithTitle:(NSString *)title initialSelection:(NSTimeZone *)index target:(id)target successAction:(SEL)successAction cancelAction:(SEL)cancelActionOrNil origin:(id)origin
{
    self = [self initWithTarget:target successAction:successAction cancelAction:cancelActionOrNil origin:origin];
    if (self) {
        self.initialTimeZone = index;
        self.title = title;
    }
    return self;
}


- (UIView *)configuredPickerView {
    [self fillContinentsAndCities];
    [self setSelectedRows];

    CGRect pickerFrame = CGRectMake(0, 40, self.viewSize.width, 216);
    UIPickerView *pickerView = [[UIPickerView alloc] initWithFrame:pickerFrame];
    pickerView.delegate = self;
    pickerView.dataSource = self;

    pickerView.showsSelectionIndicator = YES;

    [self selectCurrentLocale:pickerView];

    //need to keep a reference to the picker so we can clear the DataSource / Delegate when dismissing
    self.pickerView = pickerView;
    
    return pickerView;
}

- (void)selectCurrentLocale:(UIPickerView *)pickerView
{
    NSUInteger rowContinent = [_continents indexOfObject:self.selectedContinent];
    NSUInteger rowCity = [[self getCitiesByContinent:self.selectedContinent] indexOfObject:self.selectedCity];

    if ((rowContinent != NSNotFound) && (rowCity != NSNotFound)) // to fix some crashes from prev versions http://crashes.to/s/ecb0f15ce49
    {
        [pickerView selectRow:rowContinent inComponent:0 animated:YES];
        [pickerView reloadComponent:1];
        [pickerView selectRow:rowCity inComponent:1 animated:YES];
    }
    else
    {
        [pickerView selectRow:0 inComponent:0 animated:YES];
        [pickerView selectRow:0 inComponent:1 animated:YES];
    }
}

-(void)fillContinentsAndCities
{
    NSArray *timeZones = [NSTimeZone knownTimeZoneNames];

    NSMutableDictionary *continentsDict = [[NSMutableDictionary alloc] init];

    _continents= [[NSMutableArray alloc] init];

    [timeZones enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop)
    {
        if ( [obj isKindOfClass:[NSString class]] )
        {
            NSString *string = (NSString *) obj;
            NSArray *array = [string componentsSeparatedByString:@"/"];

            if ( [array count] == 2)
            {
                if ( continentsDict[array[0]] ) //if continent exists
                {
                    NSMutableArray *citys = continentsDict[array[0]];
                    [citys addObject:array[1]];
                }
                else //it's new continent
                {
                    NSMutableArray *mutableArray = [@[array[1]] mutableCopy];
                    continentsDict[array[0]] = mutableArray;
                    [_continents addObject:array[0]];
                }
            }
            else if (array.count == 3)
            {
                NSString *string0 = array[0];
                NSString *string1 = array[1];
                NSString *string2 = array[2];
                NSString *string3 = [string1 stringByAppendingFormat:@"/%@", string2];

                if ( continentsDict[string0] ) //if continent exists
                {
                    NSMutableArray *citys = continentsDict[string0];
                    [citys addObject:string3];
                }
                else //it's new continent
                {
                    NSMutableArray *mutableArray = [@[string3] mutableCopy];
                    continentsDict[string0] = mutableArray;
                    [_continents addObject:string0];
                }
            }
        }

    }];

    self.continentsAndCityDictionary = continentsDict;
};

- (void)setSelectedRows
{
    NSString *string;
    if (self.initialTimeZone)
        string = self.initialTimeZone.name;
    else
        string = [[NSTimeZone localTimeZone] name];

    NSArray *array = [string componentsSeparatedByString:@"/"];
    if (array.count == 1)
    {
        // Unknown time zone - appeared only in travis builds.
        self.selectedContinent = _continents[0];
        self.selectedCity = [self getCitiesByContinent:self.selectedContinent][0];
    }
    else if (array.count == 2)
    {
        self.selectedContinent = array[0];
        self.selectedCity = array[1];
    }
    else
    {
        assert(NO);
    }

}


- (void)notifyTarget:(id)target didSucceedWithAction:(SEL)successAction origin:(id)origin {

    NSString *timeZoneId = [NSString stringWithFormat:@"%@/%@", self.selectedContinent, self.selectedCity];
    NSTimeZone *timeZone = [[NSTimeZone alloc] initWithName:timeZoneId];

    if (self.onActionSheetDone) {
        _onActionSheetDone(self, timeZone);
        return;
    }
    else if (target && [target respondsToSelector:successAction]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
        [target performSelector:successAction withObject:timeZone withObject:origin];
#pragma clang diagnostic pop
        return;
    }
    NSLog(@"Invalid target/action ( %s / %s ) combination used for ActionSheetPicker", object_getClassName(target), sel_getName(successAction));
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
//
//- (NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component {
//    id obj = [self.data objectAtIndex:(NSUInteger) row];
//
//    // return the object if it is already a NSString,
//    // otherwise, return the description, just like the toString() method in Java
//    // else, return nil to prevent exception
//
//    if ([obj isKindOfClass:[NSString class]])
//        return obj;
//
//    if ([obj respondsToSelector:@selector(description)])
//        return [obj performSelector:@selector(description)];
//
//    return nil;
//}
//


/////////////////////////////////////////////////////////////////////////
#pragma mark - UIPickerViewDataSource Implementation
/////////////////////////////////////////////////////////////////////////

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
{
    return 2;
}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component
{
    // Returns
    switch (component) {
        case 0: return [_continents count];
        case 1: return [[self getCitiesByContinent:self.selectedContinent] count];
        default:break;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////
#pragma mark UIPickerViewDelegate Implementation
/////////////////////////////////////////////////////////////////////////

// returns width of column and height of row for each component. 
- (CGFloat)pickerView:(UIPickerView *)pickerView widthForComponent:(NSInteger)component
{

    switch (component) {

        case 0: return firstColumnWidth;
        case 1: return secondColumnWidth;
        default:break;
    }

    return 0;
}

- (UIView *)pickerView:(UIPickerView *)pickerView
            viewForRow:(NSInteger)row
          forComponent:(NSInteger)component
           reusingView:(UIView *)view {

    UILabel *pickerLabel = (UILabel *)view;

    if (pickerLabel == nil) {
        CGRect frame = CGRectZero;


        switch (component) {
            case 0: frame = CGRectMake(0.0, 0.0, firstColumnWidth, 32);
                break;
            case 1:
                frame = CGRectMake(0.0, 0.0, secondColumnWidth, 32);
                break;
            default:
                assert(NO);
                break;
        }

        pickerLabel = [[UILabel alloc] initWithFrame:frame];
        [pickerLabel setTextAlignment:NSTextAlignmentCenter];
        if ([pickerLabel respondsToSelector:@selector(setMinimumScaleFactor:)])
            [pickerLabel setMinimumScaleFactor:0.5];
        [pickerLabel setAdjustsFontSizeToFitWidth:YES];
        [pickerLabel setBackgroundColor:[UIColor clearColor]];
        [pickerLabel setFont:[UIFont systemFontOfSize:20]];
    }

    NSString *text;
    switch (component) {
        case 0: text = (self.continents)[(NSUInteger) row];
            break;
        case 1:
        {
            NSString *cityTitle = [self getCitiesByContinent:self.selectedContinent][(NSUInteger) row];
            NSString *timeZoneId = [NSString stringWithFormat:@"%@/%@", self.selectedContinent, cityTitle];
            NSTimeZone *timeZone = [[NSTimeZone alloc] initWithName:timeZoneId];

            NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
            [dateFormatter setTimeZone:timeZone];
            [dateFormatter setDateFormat:@"z"];
            text = [cityTitle stringByAppendingString:[NSString stringWithFormat: @" (%@)", [dateFormatter stringFromDate:[NSDate date]]]];

            break;
        }
        default:break;
    }

    [pickerLabel setText:text];

    return pickerLabel;

}

/////////////////////////////////////////////////////////////////////////

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
    switch (component) {
        case 0:
        {
            self.selectedContinent = (self.continents)[(NSUInteger) row];
            [pickerView reloadComponent:1];
            self.selectedCity = [self getCitiesByContinent:self.selectedContinent][(NSUInteger) [pickerView selectedRowInComponent:1]];
            return;
        }

        case 1:
            self.selectedCity = [self getCitiesByContinent:self.selectedContinent][(NSUInteger) row];
            return;
        default:break;
    }
}

-(NSMutableArray *)getCitiesByContinent:(NSString *)continent
{
    NSMutableArray *citiesIncontinent = _continentsAndCityDictionary[continent];
    return citiesIncontinent;
};


- (void)customButtonPressed:(id)sender {
    UIBarButtonItem *button = (UIBarButtonItem*)sender;
    NSInteger index = button.tag;
    NSAssert((index >= 0 && index < self.customButtons.count), @"Bad custom button tag: %ld, custom button count: %lu", (long)index, (unsigned long)self.customButtons.count);
    
    NSDictionary *buttonDetails = (self.customButtons)[(NSUInteger) index];
    NSAssert(buttonDetails != NULL, @"Custom button dictionary is invalid");
    
    ActionType actionType = (ActionType) [buttonDetails[kActionType] intValue];
    switch (actionType) {
        case ActionTypeValue: {
            id itemValue = buttonDetails[kButtonValue];
            if ( [itemValue isKindOfClass:[NSTimeZone class]] )
            {
                NSTimeZone *timeZone = (NSTimeZone *) itemValue;
                self.initialTimeZone = timeZone;
                [self setSelectedRows];
                [self selectCurrentLocale:(UIPickerView *) self.pickerView];
            }
            break;
        }
            
        case ActionTypeBlock:
        case ActionTypeSelector:
            [super customButtonPressed:sender];
            break;
        default:
            NSAssert(false, @"Unknown action type");
            break;
    }
}



@end