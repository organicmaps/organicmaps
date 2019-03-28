//
//  DistancePickerView.m
//
//  Created by Evan on 12/27/10.
//  Copyright 2010 NCPTT. All rights reserved.
//
//  Adapted from LabeledPickerView by Kåre Morstøl (NotTooBad Software).
//  This file only Copyright (c) 2009 Kåre Morstøl (NotTooBad Software).
//  This file only under the Eclipse public license v1.0
//  http://www.eclipse.org/legal/epl-v10.html

#import "DistancePickerView.h"


@implementation DistancePickerView


- (instancetype)initWithFrame:(CGRect)frame
{

    self = [super initWithFrame:frame];
    if ( self )
    {
        labels = [[NSMutableDictionary alloc] initWithCapacity:2];
    }
    return self;
}

- (void)addLabel:(NSString *)labeltext forComponent:(NSUInteger)component forLongestString:(NSString *)longestString
{
    labels[@(component)] = labeltext;

    NSString *keyName = [NSString stringWithFormat:@"%@_%ld", @"longestString", (unsigned long)component];

    if ( !longestString )
    {
        longestString = labeltext;
    }

    labels[keyName] = longestString;
}

- (void)updateLabel:(NSString *)labeltext forComponent:(NSUInteger)component
{

    UILabel *theLabel = (UILabel *) [self viewWithTag:component + 1];

    // Update label if it doesn’t match current label
    if ( ![theLabel.text isEqualToString:labeltext] )
    {

        NSString *keyName = [NSString stringWithFormat:@"%@_%ld", @"longestString", (unsigned long)component];
        NSString *longestString = labels[keyName];

        // Update label array with our new string value
        [self addLabel:labeltext forComponent:component forLongestString:longestString];

        // change label during fade out/in
        [UIView beginAnimations:nil context:NULL];
        [UIView setAnimationDuration:0.75];
        [UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
        theLabel.alpha = 0.00;
        theLabel.text = labeltext;
        theLabel.alpha = 1.00;
        [UIView commitAnimations];
    }

}

/** 
 Adds the labels to the view, below the selection indicator glass.
 The labels are aligned to the right side of the wheel.
 The delegate is responsible for providing enough width for both the value and the label.
 */
- (void)didMoveToWindow
{
    // exit if view is removed from the window or there are no labels.
    if ( !self.window || [labels count] == 0 )
        return;

    UIFont *labelfont = [UIFont boldSystemFontOfSize:20];

    // find the width of all the wheels combined 
    CGFloat widthofwheels = 0;
    for (int i = 0; i < self.numberOfComponents; i++)
    {
        widthofwheels += [self rowSizeForComponent:i].width;
    }

    // find the left side of the first wheel.
    // seems like a misnomer, but that will soon be corrected.
    CGFloat rightsideofwheel = (self.frame.size.width - widthofwheels) / 2;

    // cycle through all wheels
    for (int component = 0; component < self.numberOfComponents; component++)
    {
        // find the right side of the wheel
        rightsideofwheel += [self rowSizeForComponent:component].width;

        // get the text for the label. 
        // move on to the next if there is no label for this wheel.
        NSString *text = labels[@(component)];
        if ( text )
        {

            // set up the frame for the label using our longestString length
            NSString *keyName = [NSString stringWithFormat:@"%@_%@", @"longestString",
                                                           @(component)];
            NSString *longestString = labels[keyName];
            CGRect frame;

            if ( NSFoundationVersionNumber > NSFoundationVersionNumber_iOS_6_1)
            {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnavailableInDeploymentTarget"
                frame.size = [longestString sizeWithAttributes:
                                                    @{NSFontAttributeName :
                                                            labelfont}];
#pragma clang diagnostic pop
            }
            else
            {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
                frame.size = [longestString sizeWithFont:labelfont];
#pragma clang diagnostic pop
            }



            // center it vertically 
            frame.origin.y = (CGFloat) ((self.frame.size.height / 2.f) - (frame.size.height / 2.f) - 0.5f);

            // align it to the right side of the wheel, with a margin.
            // use a smaller margin for the rightmost wheel.
            frame.origin.x = rightsideofwheel - frame.size.width - (component == self.numberOfComponents - 1 ? 5 : 7);

            // set up the label. If label already exists, just get a reference to it
            BOOL addlabelView = NO;
            UILabel *label = (UILabel *) [self viewWithTag:component + 1];
            if ( !label )
            {
                label = [[UILabel alloc] initWithFrame:frame];
                addlabelView = YES;
            }

            label.text = text;
            label.font = labelfont;
            label.backgroundColor = [UIColor clearColor];
            label.shadowColor = [UIColor whiteColor];
            label.shadowOffset = CGSizeMake(0, 1);

            // Tag cannot be 0 so just increment component number to esnure we get a positive
            // NB update/remove Label methods are aware of this incrementation!
            label.tag = component + 1;

            if ( addlabelView )
            {
                /* 
                 and now for the tricky bit: adding the label to the view.
                 kind of a hack to be honest, might stop working if Apple decides to 
                 change the inner workings of the UIPickerView.
                 */
                if ( self.showsSelectionIndicator )
                {
                    // if this is the last wheel, add label as the third view from the top
                    if ( component == self.numberOfComponents - 1 ) if ( NSFoundationVersionNumber > NSFoundationVersionNumber_iOS_6_1)
                    {
                        UIView *o = [self.subviews[0] subviews][[[self.subviews[0] subviews] count] - 1];
                        UIView *subview = [o subviews][2];
                        UIView *view = [(subview.subviews)[0] subviews][1];
                        [self insertSubview:label aboveSubview:view];
                    }
                    else
                    {
                        [self insertSubview:label atIndex:[self.subviews count] - 3];
                    }
                            // otherwise add label as the 5th, 10th, 15th etc view from the top
                    else
                    {
                        if ( NSFoundationVersionNumber > NSFoundationVersionNumber_iOS_6_1)
                        {
                            [self insertSubview:label
                                   aboveSubview:[self.subviews[0] subviews][(NSUInteger) component]];
                        }
                        else
                        {
                            [self insertSubview:label aboveSubview:(self.subviews)[(NSUInteger) (5 * (component + 1))]];
                        }

                    }
                } else
                        // there is no selection indicator, so just add it to the top
                    [self addSubview:label];

            }

            if ( [self.delegate respondsToSelector:@selector(pickerView:didSelectRow:inComponent:)] )
                [self.delegate pickerView:self didSelectRow:[self selectedRowInComponent:component]
                              inComponent:component];
        }

    }

}


@end
