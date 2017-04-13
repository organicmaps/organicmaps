//
//  MPVASTModel.h
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol MPObjectMapper <NSObject>

- (id)mappedObjectFromSourceObject:(id)object;
- (Class)requiredSourceObjectClass;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

id<MPObjectMapper> MPParseArrayOf(id<MPObjectMapper> internalMapper);
id<MPObjectMapper> MPParseURLFromString();
id<MPObjectMapper> MPParseNumberFromString(NSNumberFormatterStyle numberStyle);
id<MPObjectMapper> MPParseTimeIntervalFromDurationString();
id<MPObjectMapper> MPParseClass(Class destinationClass);

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPNSStringToNSURLMapper : NSObject <MPObjectMapper>
@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPDurationStringToTimeIntervalMapper : NSObject <MPObjectMapper>
@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPStringToNumberMapper : NSObject <MPObjectMapper>

- (id)initWithNumberStyle:(NSNumberFormatterStyle)numberStyle;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPClassMapper : NSObject <MPObjectMapper>

- (id)initWithDestinationClass:(Class)destinationClass;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPNSArrayMapper : NSObject <MPObjectMapper>

- (id)initWithInternalMapper:(id<MPObjectMapper>)mapper;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPVASTModel : NSObject

+ (NSDictionary *)modelMap;
- (instancetype)initWithDictionary:(NSDictionary *)dictionary;
- (id)generateModelFromDictionaryValue:(id)value modelProvider:(id(^)(id))provider;
- (NSArray *)generateModelsFromDictionaryValue:(id)value modelProvider:(id(^)(id))provider;

@end
