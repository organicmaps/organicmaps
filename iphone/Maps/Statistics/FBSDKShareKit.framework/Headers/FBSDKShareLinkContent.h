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

#import <FBSDKShareKit/FBSDKSharingContent.h>

/*!
 @abstract A model for status and link content to be shared.
 */
@interface FBSDKShareLinkContent : NSObject <FBSDKSharingContent>

/*!
 @abstract The description of the link.
 @discussion If not specified, this field is automatically populated by information scraped from the contentURL,
 typically the title of the page.
 @return The description of the link
 */
@property (nonatomic, copy) NSString *contentDescription;

/*!
 @abstract The title to display for this link.
 @return The link title
 */
@property (nonatomic, copy) NSString *contentTitle;

/*!
 @abstract The URL of a picture to attach to this content.
 @return The network URL of an image
 */
@property (nonatomic, copy) NSURL *imageURL;

/*!
 @abstract Compares the receiver to another link content.
 @param content The other content
 @return YES if the receiver's values are equal to the other content's values; otherwise NO
 */
- (BOOL)isEqualToShareLinkContent:(FBSDKShareLinkContent *)content;

@end
