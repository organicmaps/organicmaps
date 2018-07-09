//
//  MPVASTInline.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
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
