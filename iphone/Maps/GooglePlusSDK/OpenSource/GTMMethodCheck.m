//
//  GTMMethodCheck.m
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

// Don't want any of this in release builds
#ifdef DEBUG
#import "GTMDefines.h"
#import "GTMMethodCheck.h"
#import "GTMObjC2Runtime.h"
#import <dlfcn.h>

// Checks to see if the cls passed in (or one of it's superclasses) conforms
// to NSObject protocol. Inheriting from NSObject is the easiest way to do this
// but not all classes (i.e. NSProxy) inherit from NSObject. Also, some classes
// inherit from Object instead of NSObject which is fine, and we'll count as
// conforming to NSObject for our needs.
static BOOL ConformsToNSObjectProtocol(Class cls) {
  // If we get nil, obviously doesn't conform.
  if (!cls) return NO;
  const char *className = class_getName(cls);
  if (!className) return NO;

  // We're going to assume that all Apple classes will work
  // (and aren't being checked)
  // Note to apple: why doesn't obj-c have real namespaces instead of two
  // letter hacks? If you name your own classes starting with NS this won't
  // work for you.
  // Some classes (like _NSZombie) start with _NS.
  // On Leopard we have to look for CFObject as well.
  // On iPhone we check Object as well
  if ((strncmp(className, "NS", 2) == 0)
       || (strncmp(className, "_NS", 3) == 0)
       || (strncmp(className, "__NS", 4) == 0)
       || (strcmp(className, "CFObject") == 0)
       || (strcmp(className, "__IncompleteProtocol") == 0)
       || (strcmp(className, "__ARCLite__") == 0)
       || (strcmp(className, "WebMIMETypeRegistry") == 0)
#if GTM_IPHONE_SDK
       || (strcmp(className, "Object") == 0)
       || (strcmp(className, "UIKeyboardCandidateUtilities") == 0)
#endif
    ) {
    return YES;
  }

  // iPhone and Mac OS X 10.8 with Obj-C 2 SDKs do not define the |Object|
  // class, so we instead test for the |NSObject| class.
#if GTM_IPHONE_SDK || \
    (__OBJC2__ && defined(MAC_OS_X_VERSION_10_8) && \
     MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_8)
  // Iterate through all the protocols |cls| supports looking for NSObject.
  if (cls == [NSObject class]
      || class_conformsToProtocol(cls, @protocol(NSObject))) {
    return YES;
  }
#else
  // Iterate through all the protocols |cls| supports looking for NSObject.
  if (cls == [Object class]
      || class_conformsToProtocol(cls, @protocol(NSObject))) {
    return YES;
  }
#endif

  // Recursively check the superclasses.
  return ConformsToNSObjectProtocol(class_getSuperclass(cls));
}

void GTMMethodCheckMethodChecker(void) {
  // Run through all the classes looking for class methods that are
  // prefixed with xxGMMethodCheckMethod. If it finds one, it calls it.
  // See GTMMethodCheck.h to see what it does.
#if !defined(__has_feature) || !__has_feature(objc_arc)
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
#else
  @autoreleasepool {
#endif
  int numClasses = 0;
  int newNumClasses = objc_getClassList(NULL, 0);
  int i;
  Class *classes = NULL;
  while (numClasses < newNumClasses) {
    numClasses = newNumClasses;
    classes = (Class *)realloc(classes, sizeof(Class) * numClasses);
    _GTMDevAssert(classes, @"Unable to allocate memory for classes");
    newNumClasses = objc_getClassList(classes, numClasses);
  }
  for (i = 0; i < numClasses && classes; ++i) {
    Class cls = classes[i];

    // Since we are directly calling objc_msgSend, we need to conform to
    // @protocol(NSObject), or else we will tumble into a _objc_msgForward
    // recursive loop when we try and call a function by name.
    if (!ConformsToNSObjectProtocol(cls)) {
      // COV_NF_START
      _GTMDevLog(@"GTMMethodCheckMethodChecker: Class %s does not conform to "
                 "@protocol(NSObject), so won't be checked",
                 class_getName(cls));
      continue;
      // COV_NF_END
    }
    // Since we are looking for a class method (+xxGMMethodCheckMethod...)
    // we need to query the isa pointer to see what methods it support, but
    // send the method (if it's supported) to the class itself.
    unsigned int count;
    Class metaClass = objc_getMetaClass(class_getName(cls));
    Method *methods = class_copyMethodList(metaClass, &count);
    unsigned int j;
    for (j = 0; j < count; ++j) {
      SEL selector = method_getName(methods[j]);
      const char *name = sel_getName(selector);
      if (strstr(name, "xxGTMMethodCheckMethod") == name) {
        // Check to make sure that the method we are checking comes
        // from the same image that we are in. Since GTMMethodCheckMethodChecker
        // is not exported, we should always find the copy in our local
        // image. We compare the address of it's image with the address of
        // the image which implements the method we want to check. If
        // they match we continue. This does two things:
        // a) minimizes the amount of calls we make to the xxxGTMMethodCheck
        //    methods. They should only be called once.
        // b) prevents initializers for various classes being called too early
        Dl_info methodCheckerInfo;
        if (!dladdr(GTMMethodCheckMethodChecker,
                    &methodCheckerInfo)) {
          // COV_NF_START
          // Don't know how to force this case in a unittest.
          // Certainly hope we never see it.
          _GTMDevLog(@"GTMMethodCheckMethodChecker: Unable to get dladdr info "
                "for GTMMethodCheckMethodChecker while introspecting +[%s %s]]",
                class_getName(cls), name);
          continue;
          // COV_NF_END
        }
        Dl_info methodInfo;
        if (!dladdr(method_getImplementation(methods[j]),
                    &methodInfo)) {
          // COV_NF_START
          // Don't know how to force this case in a unittest
          // Certainly hope we never see it.
          _GTMDevLog(@"GTMMethodCheckMethodChecker: Unable to get dladdr info "
                     "for %s while introspecting +[%s %s]]", name,
                     class_getName(cls), name);
          continue;
          // COV_NF_END
        }
        if (methodCheckerInfo.dli_fbase == methodInfo.dli_fbase) {
          objc_msgSend(cls, selector);
        }
      }
    }
    free(methods);
  }
  free(classes);
#if !defined(__has_feature) || !__has_feature(objc_arc)
  [pool drain];
#else
  }  // @autoreleasepool
#endif
}

#endif  // DEBUG
