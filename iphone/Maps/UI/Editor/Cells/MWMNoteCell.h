#import "MWMTableViewCell.h"

@class MWMNoteCell;

NS_ASSUME_NONNULL_BEGIN

@protocol MWMNoteCellDelegate <NSObject>

- (void)cell:(MWMNoteCell *)cell didChangeSizeAndText:(NSString *)text;
- (void)cell:(MWMNoteCell *)cell didFinishEditingWithText:(NSString *)text;

@end

@interface MWMNoteCell : MWMTableViewCell

@property(nonatomic, readonly, class) CGFloat minimalHeight;
@property(nonatomic, readonly) CGFloat cellHeight;
@property(nonatomic, readonly) CGFloat textViewContentHeight;

- (void)configWithDelegate:(id<MWMNoteCellDelegate>)delegate
                  noteText:(NSString *)text
               placeholder:(NSString *)placeholder;
- (void)updateTextViewForHeight:(CGFloat)height;

@end

NS_ASSUME_NONNULL_END
