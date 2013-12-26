//
//  SKDetailedResult.h
//  SpeechKit
//
// Copyright 2012, Nuance Communications Inc. All rights reserved.
//
// Nuance Communications, Inc. provides this document without representation
// or warranty of any kind. The information in this document is subject to
// change without notice and does not represent a commitment by Nuance
// Communications, Inc. The software and/or databases described in this
// document are furnished under a license agreement and may be used or
// copied only in accordance with the terms of such license agreement.
// Without limiting the rights under copyright reserved herein, and except
// as permitted by such license agreement, no part of this document may be
// reproduced or transmitted in any form or by any means, including, without
// limitation, electronic, mechanical, photocopying, recording, or otherwise,
// or transferred to information storage and retrieval systems, without the
// prior written permission of Nuance Communications, Inc.
//
// Nuance, the Nuance logo, Nuance Recognizer, and Nuance Vocalizer are
// trademarks or registered trademarks of Nuance Communications, Inc. or its
// affiliates in the United States and/or other countries. All other
// trademarks referenced herein are the property of their respective owners.
//

#import <Foundation/Foundation.h>
#import "SKToken.h"

/*!
 @class SKDetailedResult
 @abstract Represents a detailed recognition result.

 @discussion A SKDetailedResult expresses the recongnition result as a collection
 of indivisile smaller recongition results or SKToken objects. This provides a
 higher level of granularity when analzing the recognition results.

 @namespace SpeechKit
 */
@interface SKDetailedResult : NSObject
{
    NSArray *tokens;
    double confidenceScore;

}

/*!
 @abstract The Token objects that constitute this result.

 @discussion A Detailed result is composed of tokens. The in-order array of tokens that constitute
 this result can be retrieved using this accessor.
 */
@property (readonly, nonatomic, retain) NSArray *tokens;

/*!
 @abstract The confidence score of this result.

 @discussion Specifies the result level confidence score.
 */
@property (readonly, nonatomic) double confidenceScore;

/*!
 @abstract This method returns the token at a specific cursor postion.

 @param cursorPositionStart The specified cursor position.
                            Must be > 0

 @result Token at position.

 @discussion This method can be used to retrieve the token at the specified
 cursor position
 */
-(SKToken*) getTokenAtCursorPosition: (int)cursorPosition;

/*!
 @abstract This method returns alternative utterances for a subset of the result.

 @param cursorPositionStart The specified start cursor position.
                            Must be > 0
 @param cursorPositonEnd    The specified end cursor position.
                            Must be >=0, Must be < cursorPositionStart

 @result Array of SKAlternative objects.

 @discussion This method can be used to retrieve a list of alternative tokens
 for the tokens enclosed between cursorPositionStart and cursorPositionEnd
 */
-(NSArray*) getAlternatives: (int)cursorPositionStart: (int)cursorPositionEnd;

/*!
 @abstract This method returns the DetailedResult as a string.

 @result DetailedResult as a string.

 @discussion This method can be used to retrieve the string content of the DetailedResult.
 */
-(NSString*) toString;

+(NSArray*) createDetailedResults: (NSData*) inBinaryResults;

@end
