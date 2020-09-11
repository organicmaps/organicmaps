//
//  MPVASTMacroProcessor.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTMacroProcessor.h"
#import "MPGlobal.h"
#import "MPVASTStringUtilities.h"
#import "NSString+MPAdditions.h"

const NSTimeInterval kMPVASTMacroProcessorUnknownTimeOffset = -1;

@implementation MPVASTMacroProcessor

+ (NSURL *)macroExpandedURLForURL:(NSURL *)URL errorCode:(NSString *)errorCode
{
    return [self macroExpandedURLForURL:URL
                              errorCode:errorCode
                        videoTimeOffset:kMPVASTMacroProcessorUnknownTimeOffset
                          videoAssetURL:nil];
}

+ (NSURL *)macroExpandedURLForURL:(NSURL *)URL errorCode:(NSString *)errorCode videoTimeOffset:(NSTimeInterval)timeOffset videoAssetURL:(NSURL *)assetURL
{
    NSMutableString *URLString = [[URL absoluteString] mutableCopy];

    NSString *trimmedErrorCode = [errorCode stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
    if ([trimmedErrorCode length]) {
        [URLString replaceOccurrencesOfString:@"[ERRORCODE]" withString:errorCode options:0 range:NSMakeRange(0, [URLString length])];
        [URLString replaceOccurrencesOfString:@"%5BERRORCODE%5D" withString:errorCode options:0 range:NSMakeRange(0, [URLString length])];
    }

    if (timeOffset >= 0) {
        NSString *timeOffsetString = [MPVASTStringUtilities stringFromTimeInterval:timeOffset];
        [URLString replaceOccurrencesOfString:@"[CONTENTPLAYHEAD]" withString:timeOffsetString options:0 range:NSMakeRange(0, [URLString length])];
        [URLString replaceOccurrencesOfString:@"%5BCONTENTPLAYHEAD%5D" withString:timeOffsetString options:0 range:NSMakeRange(0, [URLString length])];
    }

    if (assetURL) {
        NSString *encodedAssetURLString = [[assetURL absoluteString] mp_URLEncodedString];
        [URLString replaceOccurrencesOfString:@"[ASSETURI]" withString:encodedAssetURLString options:0 range:NSMakeRange(0, [URLString length])];
        [URLString replaceOccurrencesOfString:@"%5BASSETURI%5D" withString:encodedAssetURLString options:0 range:NSMakeRange(0, [URLString length])];
    }

    NSString *cachebuster = [NSString stringWithFormat:@"%u", arc4random() % 90000000 + 10000000];
    [URLString replaceOccurrencesOfString:@"[CACHEBUSTING]" withString:cachebuster options:0 range:NSMakeRange(0, [URLString length])];
    [URLString replaceOccurrencesOfString:@"%5BCACHEBUSTING%5D" withString:cachebuster options:0 range:NSMakeRange(0, [URLString length])];

    return [NSURL URLWithString:URLString];
}

@end
