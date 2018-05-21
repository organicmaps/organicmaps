//
//  NSString+MPAdditions.h
//  MoPubSDK
//
//  Copyright © 2017 MoPub. All rights reserved.
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
