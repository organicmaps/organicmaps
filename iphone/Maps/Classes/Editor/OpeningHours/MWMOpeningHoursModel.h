#import "MWMOpeningHoursEditorCells.h"
#import "MWMOpeningHoursTableViewCell.h"

@protocol MWMOpeningHoursModelProtocol <NSObject>

@property (weak, nonatomic, readonly) UITableView * tableView;

@end

@interface MWMOpeningHoursModel : NSObject

@property (nonatomic, readonly) NSUInteger count;
@property (nonatomic, readonly) BOOL canAddSection;
@property (nonnull, nonatomic, readonly) NSArray<NSString *> * unhandledDays;

- (instancetype _Nullable)initWithDelegate:(id<MWMOpeningHoursModelProtocol> _Nonnull)delegate;

- (void)addSchedule;
- (void)deleteSchedule:(NSUInteger)index;

- (MWMOpeningHoursEditorCells)cellKeyForIndexPath:(NSIndexPath * _Nonnull)indexPath;

- (CGFloat)heightForIndexPath:(NSIndexPath * _Nonnull)indexPath withWidth:(CGFloat)width;
- (void)fillCell:(MWMOpeningHoursTableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath;
- (NSUInteger)numberOfRowsInSection:(NSUInteger)section;

@end
