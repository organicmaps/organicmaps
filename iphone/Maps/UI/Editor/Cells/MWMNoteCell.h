#import "MWMTableViewCell.h"

@class MWMNoteCell;

@protocol MWMNoteCelLDelegate<NSObject>

- (void)cellShouldChangeSize:(MWMNoteCell *)cell text:(NSString *)text;
- (void)cell:(MWMNoteCell *)cell didFinishEditingWithText:(NSString *)text;

@end

@interface MWMNoteCell : MWMTableViewCell

- (void)configWithDelegate:(id<MWMNoteCelLDelegate>)delegate
                  noteText:(NSString *)text
               placeholder:(NSString *)placeholder;
- (CGFloat)cellHeight;
- (void)updateTextViewForHeight:(CGFloat)height;
- (CGFloat)textViewContentHeight;
+ (CGFloat)minimalHeight;
- (void)registerObserver;

@end
