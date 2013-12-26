//
//  SKRecognition.h
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
 @abstract The SKRecognition object contains recognition results returned from
 SKRecognizer.
 */
@interface SKRecognition : NSObject
{
    NSArray *results;
    NSArray *scores;
    NSArray *detailedResults;
    NSString *suggestion;
    NSObject *data;
    NSData *binaryResults;
}

/*!
 @abstract An NSArray containing NSStrings corresponding to what may have been
 said, in order of likelihood.

 @discussion The speech recognition server returns several possible recognition
 results by order of confidence.  The first result in the array is what was
 most likely said.  The remaining list of alternatives is ranked from most
 likely to least likely.

 @deprecated Please consider using detailedResult instead
 */
@property (setter=setResult2:, nonatomic, retain) NSArray *results;

/*!
 @abstract An NSArray of NSNumbers containing the confidence scores
 corresponding to each recognition result in the results array.

 @deprecated Please consider using detailedResult instead
 */
@property (setter=setScores2:, nonatomic, retain) NSArray *scores;

/*!
 @abstract An NSArray containing SKDetailedResults corresponding to what may have been
 said, in order of likelihood.

 @discussion The speech recognition server returns several possible recognition
 results by order of confidence.  The first result in the array is what was
 most likely said.  The remaining list of alternatives is ranked from most
 likely to least likely. The SKDetailedResults provide a higher level of abstraction
 of the results obtained.
 */
@property (setter=setDetailedResults2:, nonatomic, retain) NSArray *detailedResults;

/*!
 @abstract A server-generated suggestion suitable for presentation to the user.

 @discussion This is a suggestion to the user about how he or she can improve
 recognition performance and is based on the audio received.  Examples include
 moving to a less noisy location if the environment is extremely noisy, or
 waiting a bit longer to start speaking if the beeginning of the recording seems
 truncated.  Results are often still present and may still be of useful quality.
 */
@property (setter=setSuggestion2:, nonatomic, retain) NSString *suggestion;

/*!
 @abstract Additional service-specific data.
 */
@property (setter=setData2:, nonatomic, retain) NSObject *data;

/*!
 @abstract Data used to create the SKDetailedResult instances
 */
@property (setter=setBinaryResults2:, nonatomic, retain) NSData *binaryResults;

/*!
 @abstract Returns the first NSString result in the results array.
 @result The first NSString result in the results array or nil.

 @discussion This is a convenience method to simply return the best result in the
 list of results returned from the recognition server.  It returns nil if the list
 of results is empty.
 */
- (NSString *)firstResult;

@end
