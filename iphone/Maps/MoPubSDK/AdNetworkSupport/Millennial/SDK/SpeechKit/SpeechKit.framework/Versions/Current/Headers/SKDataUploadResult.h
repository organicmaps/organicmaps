//
//  SKDataUploadResult.h
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
 @abstract The SKDataResult object contains a result for each of the data type
 that has been sent
 */
@interface SKDataResult : NSObject
{
    NSString *dataId;
    NSString *type;
    NSString *status;
    int checksum;
    int count;
    int force;
}

/*!
 @abstract Id of the data that has been sent.
 */
@property (nonatomic, retain) NSString *dataId;
/*!
 @abstract Type of the data that has been sent.
 */
@property (nonatomic, retain) NSString *type;
/*!
 @abstract Status of a particular data type uploaded: "success" or "failure"
 */
@property (nonatomic, retain) NSString *status;
/*!
 @abstract Checksum of the data that has been sent
 */
@property (nonatomic, assign) int checksum;
/*!
 @abstract Number of data items on server side
 */
@property (nonatomic, assign) int count;
/*!
 @abstract A flag to indicate if the data is synchronized, 1: out of synch, 0 otherwise
 */
@property (nonatomic, assign) int force;

@end

/*!
 @abstract The SKDataUploadResult object contains data uplpad results returned from
 SKDataUpload.
 */
@interface SKDataUploadResult : NSObject
{
    BOOL isVocRegenerated;
    NSArray *dataResults;

}

/*!
 @abstract Determine whether the voc files was re-generated (YES) or not (NO)
 */
@property (nonatomic, assign) BOOL isVocRegenerated;
/*!
 @abstract An NSArray of SKDataResults contains the data result table.
 */
@property (nonatomic, retain) NSArray *dataResults;



@end
