//
//  MPImpressionData.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, MPImpressionDataPrecision) {
    MPImpressionDataPrecisionUnknown = 0,
    MPImpressionDataPrecisionExact,
    MPImpressionDataPrecisionEstimated,
    MPImpressionDataPrecisionPublisherDefined,
    MPImpressionDataPrecisionUndisclosed,
};

@interface MPImpressionData : NSObject

- (instancetype)initWithDictionary:(NSDictionary *)impressionDataDictionary NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@property (nonatomic, copy, readonly) NSNumber * _Nullable publisherRevenue;
@property (nonatomic, copy, readonly) NSString * _Nullable impressionID;
@property (nonatomic, copy, readonly) NSString * _Nullable adUnitID;
@property (nonatomic, copy, readonly) NSString * _Nullable adUnitName;
@property (nonatomic, copy, readonly) NSString * _Nullable adUnitFormat;
@property (nonatomic, copy, readonly) NSString * _Nullable currency;
@property (nonatomic, copy, readonly) NSString * _Nullable adGroupID;
@property (nonatomic, copy, readonly) NSString * _Nullable adGroupName;
@property (nonatomic, copy, readonly) NSString * _Nullable adGroupType;
@property (nonatomic, copy, readonly) NSNumber * _Nullable adGroupPriority;
@property (nonatomic, copy, readonly) NSString * _Nullable country;
@property (nonatomic, assign, readonly) MPImpressionDataPrecision precision;
@property (nonatomic, copy, readonly) NSString * _Nullable networkName;
@property (nonatomic, copy, readonly) NSString * _Nullable networkPlacementID;
@property (nonatomic, copy, readonly) NSString * _Nullable appVersion;

@property (nonatomic, copy, readonly) NSData * _Nullable jsonRepresentation;

@end

NS_ASSUME_NONNULL_END
