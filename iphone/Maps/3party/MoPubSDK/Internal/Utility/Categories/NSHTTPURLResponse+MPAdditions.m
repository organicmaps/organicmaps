//
//  NSHTTPURLResponse+MPAdditions.m
//  MoPubSDK
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "NSHTTPURLResponse+MPAdditions.h"
#import "MPLogging.h"

NSString * const kMoPubHTTPHeaderContentType = @"Content-Type";

@implementation NSHTTPURLResponse (MPAdditions)

- (NSStringEncoding)stringEncodingFromContentType:(NSString *)contentType
{
    NSStringEncoding encoding = NSUTF8StringEncoding;

    if (![contentType length]) {
        MPLogWarn(@"Attempting to set string encoding from nil %@", kMoPubHTTPHeaderContentType);
        return encoding;
    }

    NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:@"(?<=charset=)[^;]*" options:kNilOptions error:nil];

    NSTextCheckingResult *charsetResult = [regex firstMatchInString:contentType options:kNilOptions range:NSMakeRange(0, [contentType length])];
    if (charsetResult && charsetResult.range.location != NSNotFound) {
        NSString *charset = [contentType substringWithRange:[charsetResult range]];

        // ensure that charset is not deallocated early
        CFStringRef cfCharset = (CFStringRef)CFBridgingRetain(charset);
        CFStringEncoding cfEncoding = CFStringConvertIANACharSetNameToEncoding(cfCharset);
        CFBridgingRelease(cfCharset);

        if (cfEncoding == kCFStringEncodingInvalidId) {
            return encoding;
        }
        encoding = CFStringConvertEncodingToNSStringEncoding(cfEncoding);
    }

    return encoding;
}

@end
