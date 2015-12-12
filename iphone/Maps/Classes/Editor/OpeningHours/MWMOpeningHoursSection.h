#import "MWMOpeningHoursEditorCells.h"

#include "editor/opening_hours_ui.hpp"

@class MWMOpeningHoursTableViewCell;

@protocol MWMOpeningHoursSectionProtocol <NSObject>

@property (weak, nonatomic, readonly) UITableView * tableView;

- (void)updateActiveSection:(NSUInteger)index;

- (editor::ui::TTimeTableProxy)getTimeTableProxy:(NSUInteger)index;
- (void)deleteSchedule:(NSUInteger)index;

@end

@interface MWMOpeningHoursSection : NSObject

@property (nonatomic) BOOL allDay;

@property (nonatomic, readonly) NSUInteger index;
@property (nullable, nonatomic) NSNumber * selectedRow;
@property (nonatomic, readonly) NSUInteger numberOfRows;

@property (nullable, nonatomic) NSDateComponents * cachedStartTime;
@property (nullable, nonatomic) NSDateComponents * cachedEndTime;

@property (nonatomic, readonly) BOOL canAddClosedTime;

@property (weak, nonatomic, readonly) id<MWMOpeningHoursSectionProtocol> delegate;

- (instancetype _Nullable)initWithDelegate:(id<MWMOpeningHoursSectionProtocol> _Nonnull)delegate;

- (void)refreshIndex:(NSUInteger)index;

- (MWMOpeningHoursEditorCells)cellKeyForRow:(NSUInteger)row;

- (CGFloat)heightForRow:(NSUInteger)row withWidth:(CGFloat)width;

- (NSDateComponents * _Nonnull)timeForRow:(NSUInteger)row isStart:(BOOL)isStart;

- (void)addSelectedDay:(osmoh::Weekday)day;
- (void)removeSelectedDay:(osmoh::Weekday)day;
- (BOOL)containsSelectedDay:(osmoh::Weekday)day;

- (void)addClosedTime;
- (void)removeClosedTime:(NSUInteger)row;

- (void)deleteSchedule;

- (void)refresh:(BOOL)force;

- (void)scrollIntoView;

- (BOOL)isRowSelected:(NSUInteger)row;

@end
