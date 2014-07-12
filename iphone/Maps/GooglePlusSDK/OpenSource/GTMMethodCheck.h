//
//  GTMMethodCheck.h
//  
//  Copyright 2006-2008 Google Inc.
//
//  Licensed under the Apache License, Version 2.0 (the "License"); you may not
//  use this file except in compliance with the License.  You may obtain a copy
//  of the License at
// 
//  http://www.apache.org/licenses/LICENSE-2.0
// 
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
//  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
//  License for the specific language governing permissions and limitations under
//  the License.
//

#import <Foundation/Foundation.h>
#import <stdio.h>
#import <sysexits.h>

/// A macro for enforcing debug time checks to make sure all required methods are linked in
// 
// When using categories, it can be very easy to forget to include the
// implementation of a category. 
// Let's say you had a class foo that depended on method bar of class baz, and
// method bar was implemented as a member of a category.
// You could add the following code:
// @implementation foo
// GTM_METHOD_CHECK(baz, bar)
// @end
// and the code would check to make sure baz was implemented just before main
// was called. This works for both dynamic libraries, and executables.
//
// Classes (or one of their superclasses) being checked must conform to the 
// NSObject protocol. We will check this, and spit out a warning if a class does 
// not conform to NSObject.
//
// This is not compiled into release builds.

#ifdef DEBUG

#ifdef __cplusplus
extern "C" {
#endif
  
// If you get an error for GTMMethodCheckMethodChecker not being defined, 
// you need to link in GTMMethodCheck.m. We keep it hidden so that we can have 
// it living in several separate images without conflict.
// Functions with the ((constructor)) attribute are called after all +loads
// have been called. See "Initializing Objective-C Classes" in 
// http://developer.apple.com/documentation/DeveloperTools/Conceptual/DynamicLibraries/Articles/DynamicLibraryDesignGuidelines.html#//apple_ref/doc/uid/TP40002013-DontLinkElementID_20
  
__attribute__ ((constructor, visibility("hidden"))) void GTMMethodCheckMethodChecker(void);
  
#ifdef __cplusplus
};
#endif

// This is the "magic".
// A) we need a multi layer define here so that the stupid preprocessor
//    expands __LINE__ out the way we want it. We need LINE so that each of
//    out GTM_METHOD_CHECKs generates a unique class method for the class.
#define GTM_METHOD_CHECK(class, method) GTM_METHOD_CHECK_INNER(class, method, __LINE__)
#define GTM_METHOD_CHECK_INNER(class, method, line) GTM_METHOD_CHECK_INNER_INNER(class, method, line)

// B) Create up a class method called xxGMethodCheckMethod+class+line that the 
//    GTMMethodCheckMethodChecker function can look for and call. We
//    look for GTMMethodCheckMethodChecker to enforce linkage of
//    GTMMethodCheck.m.
#define GTM_METHOD_CHECK_INNER_INNER(class, method, line) \
+ (void)xxGTMMethodCheckMethod ## class ## line { \
  void (*addr)() = GTMMethodCheckMethodChecker; \
  if (addr && ![class instancesRespondToSelector:@selector(method)] \
      && ![class respondsToSelector:@selector(method)]) { \
    fprintf(stderr, "%s:%d: error: We need method '%s' to be linked in for class '%s'\n", \
            __FILE__, line, #method, #class); \
    exit(EX_SOFTWARE); \
  } \
}

#else // !DEBUG

// Do nothing in release.
#define GTM_METHOD_CHECK(class, method)

#endif  // DEBUG
