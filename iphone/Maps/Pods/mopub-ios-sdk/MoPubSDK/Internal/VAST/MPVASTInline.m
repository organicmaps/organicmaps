//
//  MPVASTInline.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTInline.h"
#import "MPVASTCreative.h"

@implementation MPVASTInline

- (instancetype)initWithDictionary:(NSDictionary *)dictionary
{
    self = [super initWithDictionary:dictionary];
    if (self) {
        _extensions = [self generateModelFromDictionaryValue:dictionary[@"Extensions"]
                                               modelProvider:^id(NSDictionary *dictionary) {
                                                   return dictionary;
                                               }];
    }
    return self;
}

+ (NSDictionary *)modelMap
{
    return @{@"creatives":      @[@"Creatives.Creative", MPParseArrayOf(MPParseClass([MPVASTCreative class]))],
             @"errorURLs":      @[@"Error.text", MPParseArrayOf(MPParseURLFromString())],
             @"impressionURLs": @[@"Impression.text", MPParseArrayOf(MPParseURLFromString())]};
}

@end
