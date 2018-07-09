//
//  NSString+MPAdditions.m
//  MoPubSDK
//
//  Copyright © 2017 MoPub. All rights reserved.
//

#import "NSString+MPAdditions.h"

@implementation NSString (MPAdditions)

- (NSString *)mp_URLEncodedString {
    NSString *result = (NSString *)CFBridgingRelease(CFURLCreateStringByAddingPercentEscapes(NULL,
                                                                                             (CFStringRef)self,
                                                                                             NULL,
                                                                                             (CFStringRef)@"!*'();:@&=+$,/?%#[]<>",
                                                                                             kCFStringEncodingUTF8));
    return result;
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
