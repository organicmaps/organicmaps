/*
 * Author: Andreas Linde <mail@andreaslinde.de>
 *
 * Copyright (c) 2014 HockeyApp, Bit Stadium GmbH.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#import "BITHockeyAttachment.h"

/**
 Deprecated: Provides support to add binary attachments to crash reports

 This class is not needed any longer and exists for compatibility purposes with
 HockeySDK-iOS 3.5.5.

 It is a subclass of `BITHockeyAttachment` which only provides an initializer
 that is compatible with the one of HockeySDK-iOS 3.5.5.

 This is used by `[BITCrashManagerDelegate attachmentForCrashManager:]`

 @see BITHockeyAttachment
 */
@interface BITCrashAttachment : BITHockeyAttachment

/**
 Create an BITCrashAttachment instance with a given filename and NSData object

 @param filename            The filename the attachment should get
 @param crashAttachmentData The attachment data as NSData
 @param contentType         The content type of your data as MIME type

 @return An instance of BITCrashAttachment
 */
- (instancetype)initWithFilename:(NSString *)filename
             crashAttachmentData:(NSData *)crashAttachmentData
                     contentType:(NSString *)contentType;

@end
