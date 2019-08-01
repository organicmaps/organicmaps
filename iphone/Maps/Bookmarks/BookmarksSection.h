#import "MWMTypes.h"

#include "platform/location.hpp"

#include "kml/type_utils.hpp"

@protocol TableSectionDelegate <NSObject>
- (NSInteger)numberOfRows;
- (NSString *)title;
- (BOOL)canEdit;
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRow:(NSInteger)row;
- (BOOL)didSelectRow:(NSInteger)row;
- (void)deleteRow:(NSInteger)row;

@optional
-(void)updateCell:(UITableViewCell *)cell forRow:(NSInteger)row withNewLocation:(location::GpsInfo const &)gpsInfo;
@end

@class BookmarksSection;

@protocol BookmarksSectionDelegate <NSObject>
- (NSInteger)numberOfBookmarksInSection:(BookmarksSection *)bookmarkSection;
- (NSString *)titleOfBookmarksSection:(BookmarksSection *)bookmarkSection;
- (BOOL)canEditBookmarksSection:(BookmarksSection *)bookmarkSection;
- (kml::MarkId)bookmarkSection:(BookmarksSection *)bookmarkSection getBookmarkIdByRow:(NSInteger)row;
- (void)bookmarkSection:(BookmarksSection *)bookmarkSection onDeleteBookmarkInRow:(NSInteger)row;
@end

@class TracksSection;

@protocol TracksSectionDelegate <NSObject>
- (NSInteger)numberOfTracksInSection:(TracksSection *)tracksSection;
- (NSString *)titleOfTracksSection:(TracksSection *)tracksSection;
- (BOOL)canEditTracksSection:(TracksSection *)tracksSection;
- (kml::MarkId)tracksSection:(TracksSection *)tracksSection getTrackIdByRow:(NSInteger)row;
- (void)tracksSection:(TracksSection *)tracksSection onDeleteTrackInRow:(NSInteger)row;
@end

@protocol InfoSectionDelegate <NSObject>
- (UITableViewCell *)infoCellForTableView:(UITableView *)tableView;
@end

@interface BookmarksSection : NSObject <TableSectionDelegate>

@property (nullable, nonatomic) NSNumber * blockIndex;

- (instancetype)initWithDelegate:(id<BookmarksSectionDelegate>)delegate;

- (instancetype)initWithBlockIndex:(NSNumber *)blockIndex delegate:(id<BookmarksSectionDelegate>)delegate;

@end

@interface TracksSection : NSObject <TableSectionDelegate>

@property (nullable, nonatomic) NSNumber * blockIndex;

- (instancetype)initWithDelegate:(id<TracksSectionDelegate>)delegate;

- (instancetype)initWithBlockIndex:(NSNumber *)blockIndex delegate: (id<TracksSectionDelegate>)delegate;

@end

@interface InfoSection : NSObject <TableSectionDelegate>

- (instancetype)initWithDelegate:(id<InfoSectionDelegate>)delegate;

@end
