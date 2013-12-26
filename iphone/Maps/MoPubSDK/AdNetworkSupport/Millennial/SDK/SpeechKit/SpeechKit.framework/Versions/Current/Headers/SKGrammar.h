//
//  SKGrammar.h
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
#import "DataBlock.h"

/*!
 @discussion The Grammar type enumerates the different contents the grammar can take.
 Only one kind is active at a time.
 */

/*!
 @discussion The SKGrammar is used to caracterize a user defined grammar
 */
@interface SKGrammar : NSObject
{
    DataType        _type;
    int             _checksum;
    NSString*       _grammarId;
    BOOL            _loadAsLmh;
    BOOL            _setLoadAsLmh;
    int             _topicWeight;
    BOOL            _setTopicWeight;
    NSString*       _slotTag;
    NSString*       _interURI;
    NSString*       _userId;
    NSString*       _uri;
    NSMutableArray* _itemList;
}

/*!
 @abstract Returns an initialized Grammar

 @param type DataType describing the grammar
 @param grammarId Id passed to the server which identifies the grammar
 @result A specialized grammar.
 */
-(id)initWithType: (DataType) type
        grammarId: (NSString*) grammarId;

/*!
 @abstract Returns an initialized Grammar

 @param type DataType describing the grammar
 @param grammarId Id passed to the server which identifies the grammar
 @param checksum Checksum of the data previously uploaded to the server
 @result A specialized grammar.
 */
-(id)initWithType: (DataType) type
        grammarId: (NSString*) grammarId
         checksum: (int)checksum;

/*!
 @abstract Returns an initialized Grammar

 @param grammarId DataType describing the grammar
 @param uri Uri of the grammar
 @result A specialized grammar.
 */
-(id)initWithType: (NSString*) grammarId
              uri: (NSString*) uri;

/*!
 @abstract Returns an initialized Grammar

 @param grammarId DataType describing the grammar
 @param itemList list of data that will be added to the grammar
 @result A specialized grammar.
 */
-(id)initWithType: (NSString*) grammarId
         itemList: (NSMutableArray*) itemList;

/*!
 @abstract Setter for Uri

 @param uri Set Uri which is the full URI of an external grammar to be used for recognition
 */
-(void) setUri: (NSString*) uri;

/*!
 @abstract Setter for loadAsLmh

 @param loadAsLmh Flags loadAsLmh
 */
-(void) setLoadAsLmh: (BOOL) loadAsLmh;


/*!
 @abstract Setter for topicWeight

 @param weight Sets the weight for the grammar
 */
-(void) setTopicWeidht: (int) weight;


/*!
 @abstract Setter for slotTag

 @param tag New slot tag
 */
-(void) setSlotTag: (NSString*) tag;


/*!
 @abstract Setter for interUri

 @param uri Set interpretation uri, which is key to the full URI of the grammar
 to use for interpretation
 */
-(void) setInterUri: (NSString*) uri;


/*!
 @abstract Setter for checksum

 @param checksum Sets the checksum
 */
-(void) setChecksum: (int) checksum;


/*!
 @abstract Setter for userId

 @param userId Set "common_user_id" key, which is used to specify a grammar
 that is common to all users of this application (thus not device specific)
 */
-(void) setUserId: (NSString*) userId;


@end
