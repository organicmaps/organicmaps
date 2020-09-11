//
//  NSString+MPAdditions.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

@interface NSString (MPAdditions)

/*
 * Returns string with reserved/unsafe characters encoded.
 */
- (NSString *)mp_URLEncodedString;

/**
 * Attempts to convert the string into an integer value.
 * @return A valid integer or `nil` if the string is not valid.
 */
- (NSNumber *)safeIntegerValue;

@end
