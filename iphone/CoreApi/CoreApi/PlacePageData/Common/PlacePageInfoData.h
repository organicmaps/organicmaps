#import <Foundation/Foundation.h>

@class OpeningHours;

NS_ASSUME_NONNULL_BEGIN

@interface PlacePageInfoData : NSObject

@property(nonatomic, readonly, nullable) NSString *openingHoursString;
@property(nonatomic, readonly, nullable) OpeningHours *openingHours;
@property(nonatomic, readonly, nullable) NSString *phone;
@property(nonatomic, readonly, nullable) NSURL *phoneUrl;
@property(nonatomic, readonly, nullable) NSString *website;
@property(nonatomic, readonly, nullable) NSString *wikipedia;
@property(nonatomic, readonly, nullable) NSString *wikimediaCommons;
@property(nonatomic, readonly, nullable) NSString *facebook;
@property(nonatomic, readonly, nullable) NSString *instagram;
@property(nonatomic, readonly, nullable) NSString *twitter;
@property(nonatomic, readonly, nullable) NSString *vk;
@property(nonatomic, readonly, nullable) NSString *line;
@property(nonatomic, readonly, nullable) NSString *email;
@property(nonatomic, readonly, nullable) NSURL *emailUrl;
@property(nonatomic, readonly, nullable) NSString *cuisine;
@property(nonatomic, readonly, nullable) NSString *ppOperator;
@property(nonatomic, readonly, nullable) NSString *address;
@property(nonatomic, readonly, nullable) NSString *rawCoordinates;
@property(nonatomic, readonly, nullable) NSString *formattedCoordinates;
@property(nonatomic, readonly, nullable) NSString *wifiAvailable;
@property(nonatomic, readonly, nullable) NSString *level;

@end

NS_ASSUME_NONNULL_END
