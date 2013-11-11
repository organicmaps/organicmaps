/*
 * Copyright 2010-present Facebook.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import <Foundation/Foundation.h>

#import "FBDialogsParams.h"
#import "FBOpenGraphAction.h"

extern NSString *const FBPostObject;

/*!
 @class FBOpenGraphActionShareDialogParams

 @abstract
 This object is used to encapsulate state for parameters to an Open Graph share
 dialog that opens in the Facebook app.
 */
@interface FBOpenGraphActionShareDialogParams : FBDialogsParams

/*! @abstract The Open Graph action to be published. */
@property (nonatomic, retain) id<FBOpenGraphAction> action;

/*! @abstract The name of the property representing the primary target of the Open
 Graph action, which will be displayed as a preview in the dialog. */
@property (nonatomic, copy) NSString *previewPropertyName;

/*! @abstract The fully qualified type of the Open Graph action. */
@property (nonatomic, copy) NSString *actionType;

@end
