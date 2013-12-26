//
//  CDataScanner_Extensions.m
//  TouchCode
//
//  Created by Jonathan Wight on 12/08/2005.
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

#import "CDataScanner_Extensions.h"

#define LF 0x000a // Line Feed
#define FF 0x000c // Form Feed
#define CR 0x000d // Carriage Return
#define NEL 0x0085 // Next Line
#define LS 0x2028 // Line Separator
#define PS 0x2029 // Paragraph Separator

@implementation CDataScanner (CDataScanner_Extensions)

- (BOOL)scanCStyleComment:(NSString **)outComment
{
if ([self scanString:@"/*" intoString:NULL] == YES)
	{
	NSString *theComment = NULL;
	if ([self scanUpToString:@"*/" intoString:&theComment] == NO)
		[NSException raise:NSGenericException format:@"Started to scan a C style comment but it wasn't terminated."];

	if ([theComment rangeOfString:@"/*"].location != NSNotFound)
		[NSException raise:NSGenericException format:@"C style comments should not be nested."];

	if ([self scanString:@"*/" intoString:NULL] == NO)
		[NSException raise:NSGenericException format:@"C style comment did not end correctly."];

	if (outComment != NULL)
		*outComment = theComment;

	return(YES);
	}
else
	{
	return(NO);
	}
}

- (BOOL)scanCPlusPlusStyleComment:(NSString **)outComment
    {
    if ([self scanString:@"//" intoString:NULL] == YES)
        {
        unichar theCharacters[] = { LF, FF, CR, NEL, LS, PS, };
        NSCharacterSet *theLineBreaksCharacterSet = [NSCharacterSet characterSetWithCharactersInString:[NSString stringWithCharacters:theCharacters length:sizeof(theCharacters) / sizeof(*theCharacters)]];

        NSString *theComment = NULL;
        [self scanUpToCharactersFromSet:theLineBreaksCharacterSet intoString:&theComment];
        [self scanCharactersFromSet:theLineBreaksCharacterSet intoString:NULL];

        if (outComment != NULL)
            *outComment = theComment;

        return(YES);
        }
    else
        {
        return(NO);
        }
    }

- (NSUInteger)lineOfScanLocation
    {
    NSUInteger theLine = 0;
    for (const u_int8_t *C = start; C < current; ++C)
        {
        // TODO: JIW What about MS-DOS line endings you bastard! (Also other unicode line endings)
        if (*C == '\n' || *C == '\r')
            {
            ++theLine;
            }
        }
    return(theLine);
    }

- (NSDictionary *)userInfoForScanLocation
    {
    NSUInteger theLine = 0;
    const u_int8_t *theLineStart = start;
    for (const u_int8_t *C = start; C < current; ++C)
        {
        if (*C == '\n' || *C == '\r')
            {
            theLineStart = C - 1;
            ++theLine;
            }
        }

    NSUInteger theCharacter = current - theLineStart;

    NSRange theStartRange = NSIntersectionRange((NSRange){ .location = MAX((NSInteger)self.scanLocation - 20, 0), .length = 20 + (NSInteger)self.scanLocation - 20 }, (NSRange){ .location = 0, .length = self.data.length });
    NSRange theEndRange = NSIntersectionRange((NSRange){ .location = self.scanLocation, .length = 20 }, (NSRange){ .location = 0, .length = self.data.length });


    NSString *theSnippet = [NSString stringWithFormat:@"%@!HERE>!%@",
        [[[NSString alloc] initWithData:[self.data subdataWithRange:theStartRange] encoding:NSUTF8StringEncoding] autorelease],
        [[[NSString alloc] initWithData:[self.data subdataWithRange:theEndRange] encoding:NSUTF8StringEncoding] autorelease]
        ];

    NSDictionary *theUserInfo = [NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithUnsignedInteger:theLine], @"line",
        [NSNumber numberWithUnsignedInteger:theCharacter], @"character",
        [NSNumber numberWithUnsignedInteger:self.scanLocation], @"location",
        theSnippet, @"snippet",
        NULL];
    return(theUserInfo);
    }

@end
