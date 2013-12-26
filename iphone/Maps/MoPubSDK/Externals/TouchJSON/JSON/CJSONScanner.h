//
//  CJSONScanner.h
//  TouchCode
//
//  Created by Jonathan Wight on 12/07/2005.
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

#import "CDataScanner.h"

enum {
    kJSONScannerOptions_MutableContainers = 0x1,
    kJSONScannerOptions_MutableLeaves = 0x2,
};
typedef NSUInteger EJSONScannerOptions;

/// CDataScanner subclass that understands JSON syntax natively. You should generally use CJSONDeserializer instead of this class. (TODO - this could have been a category?)
@interface CJSONScanner : CDataScanner {
	BOOL strictEscapeCodes;
    id nullObject;
	NSStringEncoding allowedEncoding;
    EJSONScannerOptions options;
}

@property (readwrite, nonatomic, assign) BOOL strictEscapeCodes;
@property (readwrite, nonatomic, retain) id nullObject;
@property (readwrite, nonatomic, assign) NSStringEncoding allowedEncoding;
@property (readwrite, nonatomic, assign) EJSONScannerOptions options;

- (BOOL)setData:(NSData *)inData error:(NSError **)outError;

- (BOOL)scanJSONObject:(id *)outObject error:(NSError **)outError;
- (BOOL)scanJSONDictionary:(NSDictionary **)outDictionary error:(NSError **)outError;
- (BOOL)scanJSONArray:(NSArray **)outArray error:(NSError **)outError;
- (BOOL)scanJSONStringConstant:(NSString **)outStringConstant error:(NSError **)outError;
- (BOOL)scanJSONNumberConstant:(NSNumber **)outNumberConstant error:(NSError **)outError;

@end

extern NSString *const kJSONScannerErrorDomain /* = @"kJSONScannerErrorDomain" */;

typedef enum {

    // Fundamental scanning errors
    kJSONScannerErrorCode_NothingToScan = -11,
    kJSONScannerErrorCode_CouldNotDecodeData = -12,
    kJSONScannerErrorCode_CouldNotSerializeData = -13,
    kJSONScannerErrorCode_CouldNotSerializeObject = -14,
    kJSONScannerErrorCode_CouldNotScanObject = -15,

    // Dictionary scanning
    kJSONScannerErrorCode_DictionaryStartCharacterMissing = -101,
    kJSONScannerErrorCode_DictionaryKeyScanFailed = -102,
    kJSONScannerErrorCode_DictionaryKeyNotTerminated = -103,
    kJSONScannerErrorCode_DictionaryValueScanFailed = -104,
    kJSONScannerErrorCode_DictionaryKeyValuePairNoDelimiter = -105,
    kJSONScannerErrorCode_DictionaryNotTerminated = -106,

    // Array scanning
    kJSONScannerErrorCode_ArrayStartCharacterMissing = -201,
    kJSONScannerErrorCode_ArrayValueScanFailed = -202,
    kJSONScannerErrorCode_ArrayValueIsNull = -203,
    kJSONScannerErrorCode_ArrayNotTerminated = -204,

    // String scanning
    kJSONScannerErrorCode_StringNotStartedWithBackslash = -301,
    kJSONScannerErrorCode_StringUnicodeNotDecoded = -302,
    kJSONScannerErrorCode_StringUnknownEscapeCode = -303,
    kJSONScannerErrorCode_StringNotTerminated = -304,

    // Number scanning
    kJSONScannerErrorCode_NumberNotScannable = -401

} EJSONScannerErrorCode;
