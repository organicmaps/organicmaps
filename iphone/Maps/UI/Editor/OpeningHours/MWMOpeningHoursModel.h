#import "MWMOpeningHoursEditorCells.h"
#import "MWMOpeningHoursTableViewCell.h"
#import "MWMTextView.h"

@protocol MWMOpeningHoursModelProtocol <NSObject>

@property(nonnull, copy, nonatomic) NSString * openingHours;
@property(nullable, weak, nonatomic, readonly) UITableView * tableView;
@property(nullable, weak, nonatomic, readonly) UIView * advancedEditor;
@property(nullable, weak, nonatomic, readonly) MWMTextView * editorView;
@property(nullable, weak, nonatomic, readonly) UIButton * toggleModeButton;

@end

@interface MWMOpeningHoursModel : NSObject

@property(nonatomic, readonly) NSUInteger count;
@property(nonatomic, readonly) BOOL canAddSection;

@property(nonatomic, readonly) BOOL isValid;
@property(nonatomic) BOOL isSimpleMode;
@property(nonatomic, readonly) BOOL isSimpleModeCapable;

- (instancetype _Nullable)initWithDelegate:(id<MWMOpeningHoursModelProtocol> _Nonnull)delegate;

- (void)addSchedule;
- (void)deleteSchedule:(NSUInteger)index;

- (MWMOpeningHoursEditorCells)cellKeyForIndexPath:(NSIndexPath * _Nonnull)indexPath;

- (CGFloat)heightForIndexPath:(NSIndexPath * _Nonnull)indexPath withWidth:(CGFloat)width;
- (void)fillCell:(MWMOpeningHoursTableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath;
- (NSUInteger)numberOfRowsInSection:(NSUInteger)section;
- (editor::ui::OpeningDays)unhandledDays;

- (void)storeCachedData;
- (void)updateOpeningHours;

@end
