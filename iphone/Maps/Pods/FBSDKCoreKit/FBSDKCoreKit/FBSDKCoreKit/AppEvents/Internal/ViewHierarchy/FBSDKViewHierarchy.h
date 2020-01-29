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

#import "TargetConditionals.h"

#if !TARGET_OS_TV

#import <UIKit/UIKit.h>

typedef NS_ENUM(NSUInteger, FBCodelessClassBitmask) {
    /** Indicates that the class is subclass of UIControl */
    FBCodelessClassBitmaskUIControl     = 1 << 3,
    /** Indicates that the class is subclass of UIControl */
    FBCodelessClassBitmaskUIButton      = 1 << 4,
    /** Indicates that the class is ReactNative Button */
    FBCodelessClassBitmaskReactNativeButton = 1 << 6,
    /** Indicates that the class is UITableViewCell */
    FBCodelessClassBitmaskUITableViewCell = 1 << 7,
    /** Indicates that the class is UICollectionViewCell */
    FBCodelessClassBitmaskUICollectionViewCell = 1 << 8,
    /** Indicates that the class is UILabel */
    FBCodelessClassBitmaskLabel = 1 << 10,
    /** Indicates that the class is UITextView or UITextField*/
    FBCodelessClassBitmaskInput = 1 << 11,
    /** Indicates that the class is UIPicker*/
    FBCodelessClassBitmaskPicker = 1 << 12,
    /** Indicates that the class is UISwitch*/
    FBCodelessClassBitmaskSwitch = 1 << 13,
    /** Indicates that the class is UIViewController*/
    FBCodelessClassBitmaskUIViewController = 1 << 17,
};

extern void fb_dispatch_on_main_thread(dispatch_block_t block);
extern void fb_dispatch_on_default_thread(dispatch_block_t block);

NS_SWIFT_NAME(ViewHierarchy)
@interface FBSDKViewHierarchy : NSObject

+ (NSObject *)getParent:(NSObject *)obj;
+ (NSArray<NSObject *> *)getChildren:(NSObject *)obj;
+ (NSArray<NSObject *> *)getPath:(NSObject *)obj;
+ (NSMutableDictionary<NSString *, id> *)getDetailAttributesOf:(NSObject *)obj;

+ (NSString *)getText:(NSObject *)obj;
+ (NSString *)getHint:(NSObject *)obj;
+ (NSIndexPath *)getIndexPath:(NSObject *)obj;
+ (NSUInteger)getClassBitmask:(NSObject *)obj;
+ (UITableView *)getParentTableView:(UIView *)cell;
+ (UICollectionView *)getParentCollectionView:(UIView *)cell;
+ (NSInteger)getTag:(NSObject *)obj;
+ (NSNumber *)getViewReactTag:(UIView *)view;

+ (NSDictionary<NSString *, id> *)recursiveCaptureTree:(NSObject *)obj withObject:(NSObject *)interact;

+ (BOOL)isUserInputView:(NSObject *)obj;

@end

#endif
