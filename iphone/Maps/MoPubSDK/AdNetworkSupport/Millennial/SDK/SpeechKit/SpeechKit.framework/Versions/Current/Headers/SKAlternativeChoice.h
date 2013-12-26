//
//  SKAlternativeChoice.h
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
 @class SKAlternativeChoice
 @abstract Represents an alternative recognition result.

 @discussion An SKAlternativeChoice represents the alternative recognition results
 for a subset of the full result.

 @namespace SpeechKit
 */
@interface SKAlternativeChoice : NSObject
{
    NSArray *tokens;
}

/*!
 @abstract The Token objects that constitute this SKAlternativeChoice.

 @discussion An SKAlternativeChoice is composed of tokens. The in-order array of tokens that constitute
 this SKAlternativeChoice can be retrieved using this accessor.
 */
@property (readonly, nonatomic, retain) NSArray *tokens;

/*!
 @abstract This method returns the SKAlternativeChoice as a string.

 @result SKAlternativeChoice as a string.

 @discussion This method can be used to retrieve the string content of the SKAlternativeChoice.
 */
-(NSString*) toString;

@end
