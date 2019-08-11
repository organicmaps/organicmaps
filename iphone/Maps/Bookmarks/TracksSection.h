#import "TableSectionDataSource.h"

#include "kml/type_utils.hpp"

@class TracksSection;

@protocol TracksSectionDelegate <NSObject>

- (NSInteger)numberOfTracksInSection:(TracksSection *)tracksSection;
- (NSString *)titleOfTracksSection:(TracksSection *)tracksSection;
- (BOOL)canEditTracksSection:(TracksSection *)tracksSection;
- (kml::MarkId)tracksSection:(TracksSection *)tracksSection getTrackIdByRow:(NSInteger)row;
- (BOOL)tracksSection:(TracksSection *)tracksSection onDeleteTrackInRow:(NSInteger)row;

@end

@interface TracksSection : NSObject <TableSectionDataSource>

@property(nullable, nonatomic) NSNumber *blockIndex;

- (instancetype)initWithDelegate:(id<TracksSectionDelegate>)delegate;

- (instancetype)initWithBlockIndex:(NSNumber *)blockIndex delegate:(id<TracksSectionDelegate>)delegate;

@end
