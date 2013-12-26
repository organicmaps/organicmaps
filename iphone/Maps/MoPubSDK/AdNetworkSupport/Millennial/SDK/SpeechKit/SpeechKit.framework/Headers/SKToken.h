//
//  SKToken.h
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

/*!
 @class SKToken
 @abstract Represents a recognition token.

 @discussion A SKDetailedResult is composed of smaller recognition results or
 SKToken objects. Each SKToken has its own confidence score and can be expressed as a
 string.

 @namespace SpeechKit
 */
@interface SKToken : NSObject
{
    double confidenceScore;
}

/*!
 @abstract The confidence score of this SKToken.

 @discussion The speech recognition server returns the confidence score for each token.
 This score can be retrieved using this accessor.
 */
@property (readonly, nonatomic) double confidenceScore;

/*!
 @abstract This method returns the SKToken as a string.

 @result Token as a string.

 @discussion This method can be used to retrieve the string content of the Token.
 */
-(NSString*) toString;

@end
