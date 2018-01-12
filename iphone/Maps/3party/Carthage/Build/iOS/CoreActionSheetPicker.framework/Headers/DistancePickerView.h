//
//  DistancePickerView.h
//
//  Created by Evan on 12/27/10.
//  Copyright 2010 NCPTT. All rights reserved.
//
//  Adapted from LabeledPickerView by Kåre Morstøl (NotTooBad Software).
//  This file only Copyright (c) 2009 Kåre Morstøl (NotTooBad Software).
//  This file only under the Eclipse public license v1.0
//  http://www.eclipse.org/legal/epl-v10.html

#import <UIKit/UIKit.h>


@interface DistancePickerView : UIPickerView {
    NSMutableDictionary *labels;
}

- (void) addLabel:(NSString *)labeltext forComponent:(NSUInteger)component forLongestString:(NSString *)longestString;
- (void) updateLabel:(NSString *)labeltext forComponent:(NSUInteger)component;
@end
