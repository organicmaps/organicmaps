// Copyright 2019 Google
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#import <Foundation/Foundation.h>

/**
 * This class is a helper class for generating Multipart requests, as described in
 * http://www.w3.org/Protocols/rfc1341/7_2_Multipart.html. In the case of multiple part messages, in
 * which one or more different sets of data are combined in a single body, a "multipart"
 * Content-Type field must appear in the entity's header. The body must then contain one or more
 * "body parts," each preceded by an encapsulation boundary, and the last one followed by a closing
 * boundary. Each part starts with an encapsulation boundary, and then contains a body part
 * consisting of header area, a blank line, and a body area.
 */
@interface FIRCLSMultipartMimeStreamEncoder : NSObject

/**
 * Convenience class method to populate a NSMutableURLRequest with data from a block that takes an
 * instance of this class as input.
 */
+ (void)populateRequest:(NSMutableURLRequest *)request
    withDataFromEncoder:(void (^)(FIRCLSMultipartMimeStreamEncoder *encoder))block;

/**
 * Returns a NSString instance with multipart/form-data appended to the boundary.
 */
+ (NSString *)contentTypeHTTPHeaderValueWithBoundary:(NSString *)boundary;
/**
 * Convenience class method that returns an instance of this class
 */
+ (instancetype)encoderWithStream:(NSOutputStream *)stream andBoundary:(NSString *)boundary;
/**
 * Returns a unique boundary string.
 */
+ (NSString *)generateBoundary;
/**
 * Designated initializer
 * @param stream NSOutputStream associated with the Multipart request
 * @param boundary the unique Boundary string to be used
 */
- (instancetype)initWithStream:(NSOutputStream *)stream
                   andBoundary:(NSString *)boundary NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
/**
 * Encodes this block within the boundary on the output stream
 */
- (void)encode:(void (^)(void))block;
/**
 * Adds the contents of the file data with given Mime type anf fileName within the boundary in
 * stream
 */
- (void)addFileData:(NSData *)data
           fileName:(NSString *)fileName
           mimeType:(NSString *)mimeType
          fieldName:(NSString *)name;
/**
 * Convenience method for the method above. Converts fileURL to data and calls the above method.
 */
- (void)addFile:(NSURL *)fileURL
       fileName:(NSString *)fileName
       mimeType:(NSString *)mimeType
      fieldName:(NSString *)name;
/**
 * Adds this field and value in the stream
 */
- (void)addValue:(id)value fieldName:(NSString *)name;
/**
 * String referring to the multipart MIME type with boundary
 */
@property(nonatomic, copy, readonly) NSString *contentTypeHTTPHeaderValue;
/**
 * Length of the data written to stream
 */
@property(nonatomic, copy, readonly) NSString *contentLengthHTTPHeaderValue;

@end
