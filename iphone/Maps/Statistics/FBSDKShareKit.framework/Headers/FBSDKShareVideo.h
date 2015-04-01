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

#import <UIKit/UIKit.h>

#import <FBSDKCoreKit/FBSDKCopying.h>

/*!
 @abstract A video for sharing.
 */
@interface FBSDKShareVideo : NSObject <FBSDKCopying, NSSecureCoding>

/*!
 @abstract Convenience method to build a new video object with a videoURL.
 @param videoURL The URL to the video
 application
 */
+ (instancetype)videoWithVideoURL:(NSURL *)videoURL;

/*!
 @abstract The file URL to the video.
 @return URL that points to the location of the video on disk
 */
@property (nonatomic, copy) NSURL *videoURL;

/*!
 @abstract Compares the receiver to another video.
 @param video The other video
 @return YES if the receiver's values are equal to the other video's values; otherwise NO
 */
- (BOOL)isEqualToShareVideo:(FBSDKShareVideo *)video;

@end
