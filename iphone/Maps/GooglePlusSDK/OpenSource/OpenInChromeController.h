// Copyright 2012, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,           
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY           
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#import <Foundation/Foundation.h>

// This class is used to check if Google Chrome is installed in the system and
// to open a URL in Google Chrome either with or without a callback URL.
@interface OpenInChromeController : NSObject

// Returns a shared instance of the OpenInChromeController.
+ (OpenInChromeController *)sharedInstance;

// Returns YES if Google Chrome is installed in the user's system.
- (BOOL)isChromeInstalled;

// Opens a URL in Google Chrome.
- (BOOL)openInChrome:(NSURL *)url;

// Open a URL in Google Chrome providing a |callbackURL| to return to the app.
// URLs from the same app will be opened in the same tab unless |createNewTab|
// is set to YES.
// |callbackURL| can be nil.
// The return value of this method is YES if the URL is successfully opened.
- (BOOL)openInChrome:(NSURL *)url
     withCallbackURL:(NSURL *)callbackURL
        createNewTab:(BOOL)createNewTab;

@end
