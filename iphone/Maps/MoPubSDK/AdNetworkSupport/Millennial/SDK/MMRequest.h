//
//  MMRequest.h
//  MMSDK
//
//  Copyright (c) 2013 Millennial Media Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreLocation/CoreLocation.h>

typedef enum {
    MMEducationOther,
    MMEducationNone,
    MMEducationHighSchool,
    MMEducationInCollege,
    MMEducationSomeCollege,
    MMEducationAssociates,
    MMEducationBachelors,
    MMEducationMasters,
    MMEducationDoctorate
} MMEducation;

typedef enum {
    MMGenderOther,
    MMGenderMale,
    MMGenderFemale
} MMGender;

typedef enum {
    MMEthnicityMiddleEastern,
    MMEthnicityAsian,
    MMEthnicityBlack,
    MMEthnicityHispanic,
    MMEthnicityIndian,
    MMEthnicityNativeAmerican,
    MMEthnicityPacificIslander,
    MMEthnicityWhite,
    MMEthnicityOther
} MMEthnicity;

typedef enum {
    MMMaritalOther,
    MMMaritalSingle,
    MMMaritalRelationship,
    MMMaritalMarried,
    MMMaritalDivorced,
    MMMaritalEngaged
} MMMaritalStatus;

typedef enum {
    MMSexualOrientationOther,
    MMSexualOrientationGay,
    MMSexualOrientationStraight,
    MMSexualOrientationBisexual
} MMSexualOrientation;

@interface MMRequest : NSObject

// Creates an MMRequest object
+ (MMRequest *)request;

#pragma mark - Location Information

// Creates an MMRequest object with location
+ (MMRequest *)requestWithLocation:(CLLocation *)location;

// Set location for the ad request
@property (nonatomic, retain) CLLocation *location;

#pragma mark - Demographic Information

// Set demographic information for the ad request
@property (nonatomic, assign) MMEducation education;
@property (nonatomic, assign) MMGender gender;
@property (nonatomic, assign) MMEthnicity ethnicity;
@property (nonatomic, assign) MMMaritalStatus maritalStatus;
@property (nonatomic, assign) MMSexualOrientation orientation;
@property (nonatomic, retain) NSNumber *age;
@property (nonatomic, copy) NSString *zipCode;

#pragma mark - Contextual Information

// Set an array of keywords (must be NSString values)
@property (nonatomic, retain) NSMutableArray *keywords;

// Add keywords one at a time to the array
- (void)addKeyword:(NSString *)keyword;

#pragma mark - Additional Information

// Set additional parameters for the ad request. Value must be NSNumber or NSString.
- (void)setValue:(id)value forKey:(NSString *)key;

#pragma mark - Request parameters (read-only)
@property (nonatomic, retain, readonly) NSMutableDictionary *dataParameters;

@end
