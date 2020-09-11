//
//  MPVASTWrapper.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
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
