//
//  PDXWrapper.h
//  Acadia
//
//  Created by Pete Lyons on 3/26/10.
//  Copyright 2010 Nuance Communications Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#define _IPHONE
#include <nmsp_core/nmsp_core_pdx.h>

@class PDXDictionary;

#pragma mark -
@interface PDXSequence : NSObject {
	nmsp_core_Sequence* seq;
	BOOL own;
}
-(id) init;
-(id) initWithNmspNmasSequence:(nmsp_core_Sequence*) sequence;
-(BOOL) isNullType:(int) offset;
-(int) getInteger:(int) offset;
-(NSString*) getString:(int) offset;
-(PDXSequence*) getSequence:(int) offset;
-(PDXDictionary*) getDictionary:(int) offset;
-(void) dealloc;
-(int) count;

-(void) addInteger:(int) i;
-(void) addUtf8String:(const char* ) str;
-(void) addAsciiString:(const char* ) str;
-(void) addSequence:(PDXSequence*) seq;
-(void) addDictionary:(PDXDictionary*) dict;
-(void) addBytes:(const char* ) str length:(long)len;
-(void) addNull;

-(nmsp_core_Sequence*) detach;
-(nmsp_core_Sequence*) peek;
@end


#pragma mark -
@interface PDXDictionary : NSObject {
	nmsp_core_Dictionary* dict;
	BOOL own;
}
-(id) init;
-(id) initWithNmspNmasDictionary:(nmsp_core_Dictionary*) dictionary;
+(id) createFromNativeDictionary:(NSDictionary*) nativeDict;
-(id) getNative:(NSString*)key;
-(int) getInteger:(NSString*) key;
-(NSString*) getString:(NSString*) key;
-(NSData*) getBytes:(NSString*) key;
-(PDXSequence*) getSequence:(NSString*) key;
-(PDXDictionary*) getDictionary:(NSString*) key;
-(NSData*) getData:(NSString*) key;
-(void) dealloc;
-(BOOL) keyExists:(NSString*) key;

-(void) addInteger:(int) i atKey: (NSString*) key;
-(void) addUtf8String:(const char* ) str atKey: (NSString*) key;
-(void) addAsciiString:(const char* ) str atKey: (NSString*) key;
-(void) addSequence:(PDXSequence*) seq atKey: (NSString*) key;
-(void) addDictionary:(PDXDictionary*) dict atKey: (NSString*) key;
-(void) addBytes:(const char*)data length:(long)len atKey: (NSString*) key;
-(void) addNullAtKey: (NSString*) key;

-(nmsp_core_Dictionary*) detach;
-(nmsp_core_Dictionary*) peek;

-(void) fillWithKeys:(NSArray*) keys andValues:(NSArray*) vals;

@end
