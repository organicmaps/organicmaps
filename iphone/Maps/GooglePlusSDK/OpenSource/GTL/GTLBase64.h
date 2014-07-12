/* Copyright (c) 2012 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import <Foundation/Foundation.h>

NSData *GTLDecodeBase64(NSString *base64Str);
NSString *GTLEncodeBase64(NSData *data);

// "Web-safe" encoding substitutes - and _ for + and / in the encoding table,
// per http://www.ietf.org/rfc/rfc4648.txt section 5.

NSData *GTLDecodeWebSafeBase64(NSString *base64Str);
NSString *GTLEncodeWebSafeBase64(NSData *data);
