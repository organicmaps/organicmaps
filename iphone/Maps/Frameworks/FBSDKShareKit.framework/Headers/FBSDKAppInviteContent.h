// Copyright (c) 2014-present, Facebook, Inc. All rights reserved.
//
// You are hereby granted a non-exclusive, worldwide, royalty-free license to use,
// copy, modify, and distribute this software in source code or binary form for use
// in connection with the web services and APIs provided by Facebook.
//
// As with any software that integrates with the Facebook platform, your use of
// this software is subject to the Facebook Developer Principles and Policies
// [http://developers.facebook.com/policy/]. This copyright notice shall be
// included in all copies or substantial portions of the software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#import <Foundation/Foundation.h>

#import <FBSDKCoreKit/FBSDKCopying.h>

/*!
 @abstract A model for app invite.
 */
@interface FBSDKAppInviteContent : NSObject <FBSDKCopying, NSSecureCoding>

/*!
 @abstract A URL to a preview image that will be displayed with the app invite

 @discussion This is optional.  If you don't include it a fallback image will be used.
*/
@property (nonatomic, copy) NSURL *appInvitePreviewImageURL;

/*!
 @abstract An app link target that will be used as a target when the user accept the invite.

 @discussion This is a requirement.
 */
@property (nonatomic, copy) NSURL *appLinkURL;

/*!
 @deprecated Use `appInvitePreviewImageURL` instead.
 */
@property (nonatomic, copy) NSURL *previewImageURL __attribute__ ((deprecated("use appInvitePreviewImageURL instead")));

/*!
 @abstract Compares the receiver to another app invite content.
 @param content The other content
 @return YES if the receiver's values are equal to the other content's values; otherwise NO
 */
- (BOOL)isEqualToAppInviteContent:(FBSDKAppInviteContent *)content;

@end
