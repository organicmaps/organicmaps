//
//  SKEarcon.h
//  SpeechKit
//
//  Created by Stephen Laverty on 8/12/10.
//  Copyright 2012 Nuance. All rights reserved.
//

#import <Foundation/Foundation.h>


/*!
 @abstract Type for Earcon type definitions
 */
typedef NSUInteger SKEarconType;

/*!
 @enum Earcon type definitions
 @abstract These constants define the various Earcon types for the earcons set in
 the detection parameter of the method setEarcon:forType:.
 @constant SKStartRecordingEarconType Earcon to play before starting record.
 @constant SKStopRecordingEarconType Earcon to play after stopping record.
 @constant SKCancelRecordingEarconType Earcon to play when recognition is canceled.
 */
enum {
	SKStartRecordingEarconType = 1,
	SKStopRecordingEarconType = 2,
	SKCancelRecordingEarconType = 3,
};

/*!
 @discussion The SKEarcon class generates an earcon object to be played in a recognition
 session. It is set using the static method setEarcon:forType.
 */
@interface SKEarcon : NSObject {

}

/*!
 @abstract This method initializes an earcon from an audio path.
 @param path Path of the audio file
 @result SKEarcon object is returned.
 @discussion The format of the audio file (MP3, WAV, etc.) must be supported on the device.
 The audio file is considered as part of the client resources.
 */
- (id)initWithContentsOfFile:(NSString*)path;

/*!
 @abstract This method initializes an earcon from an audio path.
 @param name Name of the audio file
 @result SKEarcon object instance is returned.
 @discussion The format of the audio file (MP3, WAV, etc.) must be supported on the device.
 The audio file is considered as part of the client resources. This static method allocates memory
 and returns a new instance of the Earcon object.
 */
+ (id)earconWithName:(NSString*)name;
@end
