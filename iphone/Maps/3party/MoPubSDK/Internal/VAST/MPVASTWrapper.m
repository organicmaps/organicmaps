//
//  MPVASTWrapper.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPVASTWrapper.h"
#import "MPVASTCreative.h"

@interface MPVASTWrapper ()

@property (nonatomic, readwrite) MPVASTResponse *wrappedVASTResponse;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPVASTWrapper

- (instancetype)initWithDictionary:(NSDictionary *)dictionary
{
    self = [super initWithDictionary:dictionary];
    if (self) {
        _extensions = [self generateModelsFromDictionaryValue:dictionary[@"Extensions"][@"Extension"]
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
             @"impressionURLs": @[@"Impression.text", MPParseArrayOf(MPParseURLFromString())],
             //             @"extensions":     @[@"Extensions.Extension"],
             @"VASTAdTagURI":   @[@"VASTAdTagURI.text", MPParseURLFromString()]};
}

@end
