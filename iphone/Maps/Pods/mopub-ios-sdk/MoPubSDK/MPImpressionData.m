//
//  MPImpressionData.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPImpressionData.h"
#import "MPAdServerKeys.h"

static NSString * const kPrecisionOptionPublisherDefinedKey = @"publisher_defined";
static NSString * const kPrecisionOptionExactKey            = @"exact";
static NSString * const kPrecisionOptionEstimatedKey        = @"estimated";
static NSString * const kPrecisionOptionUndisclosedKey      = @"undisclosed";

@interface MPImpressionData ()

@property (nonatomic, assign, readwrite) MPImpressionDataPrecision precision;
@property (nonatomic, copy, readwrite) NSData * jsonRepresentation;

@property (nonatomic, copy) NSDictionary * impressionDataDictionary;
@property (nonatomic, assign) BOOL isPrecisionSet;

@end

@implementation MPImpressionData

- (instancetype)initWithDictionary:(NSDictionary *)impressionDataDictionary {
    if (self = [super init]) {
        _impressionDataDictionary = impressionDataDictionary;
    }

    return self;
}

/*
 @c impressionDataDictionary must be stored for the purposes of assembling the JSON
 (in case ad server includes fields that this model class doesn't know about), so since
 dictionary reads are O(1) and the property values are already stored in the dictionary,
 just override the property getters and return values from the dictionary, rather than
 storing duplicates of the values in memory.
 */

- (NSString *)impressionID {
    return [self nullableImpressionDataObjectForKey:kImpressionDataImpressionIDKey];
}

- (NSString *)adUnitID {
    return [self nullableImpressionDataObjectForKey:kImpressionDataAdUnitIDKey];
}

- (NSString *)adUnitName {
    return [self nullableImpressionDataObjectForKey:kImpressionDataAdUnitNameKey];
}

- (NSString *)adUnitFormat {
    return [self nullableImpressionDataObjectForKey:kImpressionDataAdUnitFormatKey];
}

- (NSString *)adGroupID {
    return [self nullableImpressionDataObjectForKey:kImpressionDataAdGroupIDKey];
}

- (NSString *)adGroupName {
    return [self nullableImpressionDataObjectForKey:kImpressionDataAdGroupNameKey];
}

- (NSString *)adGroupType {
    return [self nullableImpressionDataObjectForKey:kImpressionDataAdGroupTypeKey];
}

- (NSNumber *)adGroupPriority {
    return [self nullableImpressionDataObjectForKey:kImpressionDataAdGroupPriorityKey];
}

- (NSString *)currency {
    return [self nullableImpressionDataObjectForKey:kImpressionDataCurrencyKey];
}

- (NSString *)country {
    return [self nullableImpressionDataObjectForKey:kImpressionDataCountryKey];
}

- (NSString *)networkName {
    return [self nullableImpressionDataObjectForKey:kImpressionDataNetworkNameKey];
}

- (NSString *)networkPlacementID {
    return [self nullableImpressionDataObjectForKey:kImpressionDataNetworkPlacementIDKey];
}

- (NSString *)appVersion {
    return [self nullableImpressionDataObjectForKey:kImpressionDataAppVersionKey];
}

- (NSNumber *)publisherRevenue {
    return [self nullableImpressionDataObjectForKey:kImpressionDataPublisherRevenueKey];
}

- (MPImpressionDataPrecision)precision {
    // Return the precision value if it was already set
    if (self.isPrecisionSet) {
        return _precision;
    }

    // If not, set it
    self.isPrecisionSet = YES;

    NSString * precisionString = [self nullableImpressionDataObjectForKey:kImpressionDataPrecisionKey];

    // If the precision string is nil, the precision is unknown, and no other checks are required.
    if (precisionString == nil) {
        self.precision = MPImpressionDataPrecisionUnknown;
        return _precision;
    }

    // If the precision string is equal to some known string, the precision is known. Otherwise, it's unknown.
    if ([precisionString isEqualToString:kPrecisionOptionExactKey]) {
        self.precision = MPImpressionDataPrecisionExact;
    } else if ([precisionString isEqualToString:kPrecisionOptionEstimatedKey]) {
        self.precision = MPImpressionDataPrecisionEstimated;
    } else if ([precisionString isEqualToString:kPrecisionOptionPublisherDefinedKey]) {
        self.precision = MPImpressionDataPrecisionPublisherDefined;
    } else if ([precisionString isEqualToString:kPrecisionOptionUndisclosedKey]) {
        self.precision = MPImpressionDataPrecisionUndisclosed;
    } else {
        self.precision = MPImpressionDataPrecisionUnknown;
    }

    return _precision;
}

// This method gets the object for the @c key given from @c impressionDataDictionary. If the object is of type @c NSNull, this method returns @c nil.
// This way, getters will return @c nil for null JSON values. This method does not mutate the dictionary to remove @c NSNull objects.
- (id)nullableImpressionDataObjectForKey:(id)key {
    id object = self.impressionDataDictionary[key];

    if ([object isKindOfClass:[NSNull class]]) {
        return nil;
    }

    return object;
}

- (NSData *)jsonRepresentation {
    // If the JSON representation was already computed, do not do it again.
    if (_jsonRepresentation != nil) {
        return _jsonRepresentation;
    }

    // If this is the first time accessing the JSON representation, compute it.
    NSError * error = nil;
    NSData * jsonData = [NSJSONSerialization dataWithJSONObject:self.impressionDataDictionary
                                                        options:0
                                                          error:&error];
    if (error == nil) {
        self.jsonRepresentation = jsonData;
    }

    return _jsonRepresentation;
}

- (NSString *)description {
    NSString * precisionString;
    switch (self.precision) {
        case MPImpressionDataPrecisionExact:
            precisionString = kPrecisionOptionExactKey;
            break;
        case MPImpressionDataPrecisionEstimated:
            precisionString = kPrecisionOptionEstimatedKey;
            break;
        case MPImpressionDataPrecisionPublisherDefined:
            precisionString = kPrecisionOptionPublisherDefinedKey;
            break;
        case MPImpressionDataPrecisionUndisclosed:
            precisionString = kPrecisionOptionUndisclosedKey;
            break;
        default:
            precisionString = nil;
            break;
    }

    NSDictionary * jsonRepresentationDeserialized = [NSJSONSerialization JSONObjectWithData:self.jsonRepresentation
                                                                                    options:0
                                                                                      error:nil];

    return [NSString stringWithFormat:@"Impression Data %@:\n\nImpression ID: %@\nApp Version: %@\nPublisher Revenue: %@\nCurrency: %@\nAd Unit ID: %@\nAd Unit Name: %@\nAd Unit Format: %@\nAd Group ID: %@\nAd Group Name: %@\nAd Group Type: %@\nAd Group Priority: %@\nPrecision: %@\nCountry: %@\nNetwork Name: %@\nNetwork Placement ID: %@\n\nJSON Representation:\n%@",
            [super description],
            self.impressionID,
            self.appVersion,
            self.publisherRevenue,
            self.currency,
            self.adUnitID,
            self.adUnitName,
            self.adUnitFormat,
            self.adGroupID,
            self.adGroupName,
            self.adGroupType,
            self.adGroupPriority,
            precisionString,
            self.country,
            self.networkName,
            self.networkPlacementID,
            jsonRepresentationDeserialized];
}

@end
