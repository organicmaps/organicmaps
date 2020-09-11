//
//  NSString+MPAdditions.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "NSString+MPAdditions.h"

@implementation NSString (MPAdditions)

- (NSString *)mp_URLEncodedString {
    NSString *charactersToEscape = @"!*'();:@&=+$,/?%#[]<>";
    NSCharacterSet *allowedCharacters = [[NSCharacterSet characterSetWithCharactersInString:charactersToEscape] invertedSet];
    return [self stringByAddingPercentEncodingWithAllowedCharacters:allowedCharacters];
}

- (NSNumber *)safeIntegerValue {
    // Reusable number formatter since reallocating this is expensive.
    static NSNumberFormatter * formatter = nil;
    if (formatter == nil) {
        formatter = [[NSNumberFormatter alloc] init];
        formatter.numberStyle = NSNumberFormatterNoStyle;
    }

    return [formatter numberFromString:self];
}

@end
