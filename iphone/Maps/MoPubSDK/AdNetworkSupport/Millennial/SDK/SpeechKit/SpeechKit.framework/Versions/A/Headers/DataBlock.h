//
//  DataBlock.h
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
 @enum Action type for data uploading

 @constant ADD Add a list of contacts, words of a data type
 @constant REMOVE Delete a list of contacts, words of a data type
 @constant CLEARALL Clear all the data of a data type
 */
typedef enum {
    ADD = 0,
    REMOVE,
    CLEARALL,
} ActionType;

/*!
 @enum Data type for data uploading

 @constant CONTACTS contacts data type
 @constant CUSTOMWORDS custom_words data type
 @constant HISTORY history data type
 @constant URI uri data type
 @constant PREDEFINED_STATIC_GRAMMAR predefined static grammar data type
 @constant INSTANT_ITEM_LIST instant item list data type
 @constant ENUM_DATATYPE_COUNT number of items in the enumeration
 */
typedef enum {
    CONTACTS = 0,
    CUSTOMWORDS,
    HISTORY,
    URI,
    PREDEFINED_STATIC_GRAMMARS,
    INSTANT_ITEM_LIST,
    ENUM_DATATYPE_COUNT
} DataType;

/*!
 @class Contact
 @abstract Contact class specifies contact data structure for data uploading
 */
@interface Contact : NSObject {
    NSString *firstname;
    NSString *firstname_phonetic;
    NSString *lastname;
    NSString *lastname_phonetic;
    NSString *fullname;
    NSString *nickname;
    NSString *middlename;
    NSString *middlename_phonetic;
    NSString *company;
}

/*!
 @abstract Contact first name
 */
@property (nonatomic, retain) NSString *firstname;
/*!
 @abstract Contact first name in phonetic
 */
@property (nonatomic, retain) NSString *firstname_phonetic;
/*!
 @abstract Contact last name
 */
@property (nonatomic, retain) NSString *lastname;
/*!
 @abstract Contact last name in phonetic
 */
@property (nonatomic, retain) NSString *lastname_phonetic;
/*!
 @abstract Contact full name
 */
@property (nonatomic, retain) NSString *fullname;
/*!
 @abstract Contact nick name
 */
@property (nonatomic, retain) NSString *nickname;
/*!
 @abstract Contact middle name
 */
@property (nonatomic, retain) NSString *middlename;
/*!
 @abstract Contact middle name in phonetic
 */
@property (nonatomic, retain) NSString *middlename_phonetic;
/*!
 @abstract Contact company infomation
 */
@property (nonatomic, retain) NSString *company;

/*!
 @abstract Returns an initialized contact object

 @param firstName Contact first name
 @param middleName Contact middle name
 @param lastName Contact last name

 @result A contact object
 */
- (id)initWithFirstname:(NSString *)firstName middlename:(NSString *)middleName lastname:(NSString *)lastName;

/*!
 @abstract Returns a checksum for a contact considering all valide elements
 */
- (int)getCheckSum;

@end


/*!
 @class Action
 @abstract Action class specifies action data structure for data uploading
 */
@interface Action : NSObject {
    ActionType action;
    NSMutableArray *contacts;
    NSMutableArray *words;
}

/*!
 @abstract Action type
*/
@property (nonatomic, assign) ActionType action;
/*!
 @abstract Contact list in this action
 */
@property (nonatomic, retain) NSMutableArray *contacts;
/*!
 @abstract Word list in this action
 */
@property (nonatomic, retain) NSMutableArray *words;


/*!
 @abstract Returns an initialized action object

 @param actionType one value of ActionType

 @result An action object
 */
- (id)initWithType:(ActionType)actionType;
/*!
 @abstract Returns a checksum for an action considering all valide elements
 */
- (int)getCheckSum;
/*!
 @abstract Add a contact to this action
 @param contact Contact object to added
 */
- (void)addContact:(Contact *)contact;
/*!
 @abstract Remove a contact from this action
 @param contact Contact object to be removed
 */
- (void)removeContact:(Contact *)contact;
/*!
 @abstract Add a string to this action
 @param word NSString object to added
 */
- (void)addWord:(NSString *)word;
/*!
 @abstract Remove a string from this action
 @param word NSString object to be removed
 */
- (void)removeWord:(NSString *)word;

@end

/*!
 @class Data
 @abstract Data class specifies an element in the sequence of DataBlock to be
 sent to server
 */
@interface  Data  : NSObject {
    NSString *dataId;
    DataType type;
    NSMutableArray *actions;
}

/*!
 @abstract Data type
 */
@property (nonatomic, assign) DataType type;
/*!
 @abstract data Id
 */
@property (nonatomic, retain) NSString *dataId;
/*!
 @abstract Action list in this data
 */
@property (nonatomic, retain) NSMutableArray *actions;

/*!
 @abstract Returns an initialized data object

 @param dataIdentity data identification
 @param datatype one value of DataType

 @result A data object
 */
- (id)initWithId:(NSString *)dataIdentity datatype:(DataType)datatype;
/*!
 @abstract Returns a checksum for a data considering all valide elements
 */
- (int)getCheckSum;
/*!
 @abstract Add an action to this data
 @param action Action object to added
 */
- (void)addAction:(Action *)action;
/*!
 @abstract Remove an action from this data
 @param action Action object to be removed
 */
- (void)removeAction:(Action *)action;

/*!
 @abstract Returns a string representing the type, "contacts", "custom_words", or "history"
 @result A NSString object representing the type.
 */
- (NSString *)typeString;

@end

 /*!
 @class DataBlock
 @abstract DataBlock class specifies a data structure as a parameter value
 sent with "DATA_BLOCK" or "UPLOAD_DONE" to server
 */
@interface DataBlock : NSObject {
    BOOL deleteall;
    NSString *userId;
    NSMutableArray *datalist;
}

/*!
 @abstract deleteall

 @discussion Set delete_all key to tell server wipe out the entire user information
 */
@property (nonatomic, assign) BOOL deleteall;
/*!
 @abstract user Id

 @discussion Set "common_user_id" key, which can be used by an application that
 is uploading data that will be common for all users of this application.
 */
@property (nonatomic, retain) NSString *userId;
/*!
 @abstract Data list in this data block
 */
@property (nonatomic, retain) NSMutableArray *datalist;

/*!
 @abstract Returns a checksum for a data block considering all valide elements
 */
- (int)getCheckSum;
/*!
 @abstract Add a data element to this data block
 @param data Data object to added
 */
- (void)addData:(Data *)data;
/*!
 @abstract Remove a data from this data block
 @param data Data object to be removed
 */
- (void)removeData:(Data *)data;

@end
