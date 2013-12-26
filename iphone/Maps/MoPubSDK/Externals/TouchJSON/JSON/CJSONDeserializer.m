//
//  CJSONDeserializer.m
//  TouchCode
//
//  Created by Jonathan Wight on 12/15/2005.
//  Copyright 2005 toxicsoftware.com. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person
//  obtaining a copy of this software and associated documentation
//  files (the "Software"), to deal in the Software without
//  restriction, including without limitation the rights to use,
//  copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following
//  conditions:
//
//  The above copyright notice and this permission notice shall be
//  included in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
//  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//  OTHER DEALINGS IN THE SOFTWARE.
//

#import "CJSONDeserializer.h"

#import "CJSONScanner.h"
#import "CDataScanner.h"

NSString *const kJSONDeserializerErrorDomain  = @"CJSONDeserializerErrorDomain";

@interface CJSONDeserializer ()
@end

@implementation CJSONDeserializer

@synthesize scanner;
@synthesize options;

+ (CJSONDeserializer *)deserializer
    {
    return([[[self alloc] init] autorelease]);
    }

- (id)init
    {
    if ((self = [super init]) != NULL)
        {
        }
    return(self);
    }

- (void)dealloc
    {
    [scanner release];
    scanner = NULL;
    //
    [super dealloc];
    }

#pragma mark -

- (CJSONScanner *)scanner
    {
    if (scanner == NULL)
        {
        scanner = [[CJSONScanner alloc] init];
        }
    return(scanner);
    }

- (id)nullObject
    {
    return(self.scanner.nullObject);
    }

- (void)setNullObject:(id)inNullObject
    {
    self.scanner.nullObject = inNullObject;
    }

#pragma mark -

- (NSStringEncoding)allowedEncoding
    {
    return(self.scanner.allowedEncoding);
    }

- (void)setAllowedEncoding:(NSStringEncoding)inAllowedEncoding
    {
    self.scanner.allowedEncoding = inAllowedEncoding;
    }

#pragma mark -

- (id)deserialize:(NSData *)inData error:(NSError **)outError
    {
    if (inData == NULL || [inData length] == 0)
        {
        if (outError)
            *outError = [NSError errorWithDomain:kJSONDeserializerErrorDomain code:kJSONScannerErrorCode_NothingToScan userInfo:NULL];

        return(NULL);
        }
    if ([self.scanner setData:inData error:outError] == NO)
        {
        return(NULL);
        }
    id theObject = NULL;
    if ([self.scanner scanJSONObject:&theObject error:outError] == YES)
        return(theObject);
    else
        return(NULL);
    }

- (id)deserializeAsDictionary:(NSData *)inData error:(NSError **)outError
    {
    if (inData == NULL || [inData length] == 0)
        {
        if (outError)
            *outError = [NSError errorWithDomain:kJSONDeserializerErrorDomain code:kJSONScannerErrorCode_NothingToScan userInfo:NULL];

        return(NULL);
        }
    if ([self.scanner setData:inData error:outError] == NO)
        {
        return(NULL);
        }
    NSDictionary *theDictionary = NULL;
    if ([self.scanner scanJSONDictionary:&theDictionary error:outError] == YES)
        return(theDictionary);
    else
        return(NULL);
    }

- (id)deserializeAsArray:(NSData *)inData error:(NSError **)outError
    {
    if (inData == NULL || [inData length] == 0)
        {
        if (outError)
            *outError = [NSError errorWithDomain:kJSONDeserializerErrorDomain code:kJSONScannerErrorCode_NothingToScan userInfo:NULL];

        return(NULL);
        }
    if ([self.scanner setData:inData error:outError] == NO)
        {
        return(NULL);
        }
    NSArray *theArray = NULL;
    if ([self.scanner scanJSONArray:&theArray error:outError] == YES)
        return(theArray);
    else
        return(NULL);
    }

@end
