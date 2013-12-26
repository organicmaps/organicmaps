//
//  SKGenericResult.h
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
 @abstract The SKGenericResult object contains command results returned from
 SKGenericCommand.
 */
@interface SKGenericResult : NSObject
{
    NSString *status;
    NSObject *data;
}

/*!
 @abstract A server-generated result from a single command with no complex result.

 @discussion The command status could be "successful", "ok" etc.
 */
@property (nonatomic, retain) NSString *status;

/*!
 @abstract Additional service-specific data.
 */
@property (nonatomic, retain) NSObject *data;


@end
