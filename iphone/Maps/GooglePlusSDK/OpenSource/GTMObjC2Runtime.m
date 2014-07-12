//
//  GTMObjC2Runtime.m
//
//  Copyright 2007-2008 Google Inc.
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

#import "GTMObjC2Runtime.h"

#if GTM_MACOS_SDK && (MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5)
#import <stdlib.h>
#import <string.h>

Class object_getClass(id obj) {
  if (!obj) return NULL;
  return obj->isa;
}

const char *class_getName(Class cls) {
  if (!cls) return "nil";
  return cls->name;
}

BOOL class_conformsToProtocol(Class cls, Protocol *protocol) {
  // We intentionally don't check cls as it crashes on Leopard so we want
  // to crash on Tiger as well.
  // I logged
  // Radar 5572978 class_conformsToProtocol crashes when arg1 is passed as nil
  // because it seems odd that this API won't accept nil for cls considering
  // all the other apis will accept nil args.
  // If this does get fixed, remember to enable the unit tests.
  if (!protocol) return NO;

  struct objc_protocol_list *protos;
  for (protos = cls->protocols; protos != NULL; protos = protos->next) {
    for (long i = 0; i < protos->count; i++) {
      if ([protos->list[i] conformsTo:protocol]) {
        return YES;
      }
    }
  }
  return NO;
}

Class class_getSuperclass(Class cls) {
  if (!cls) return NULL;
  return cls->super_class;
}

BOOL class_respondsToSelector(Class cls, SEL sel) {
  return class_getInstanceMethod(cls, sel) != nil;
}

Method *class_copyMethodList(Class cls, unsigned int *outCount) {
  if (!cls) return NULL;

  unsigned int count = 0;
  void *iterator = NULL;
  struct objc_method_list *mlist;
  Method *methods = NULL;
  if (outCount) *outCount = 0;

  while ( (mlist = class_nextMethodList(cls, &iterator)) ) {
    if (mlist->method_count == 0) continue;
    methods = (Method *)realloc(methods,
                                sizeof(Method) * (count + mlist->method_count + 1));
    if (!methods) {
      //Memory alloc failed, so what can we do?
      return NULL;  // COV_NF_LINE
    }
    for (int i = 0; i < mlist->method_count; i++) {
      methods[i + count] = &mlist->method_list[i];
    }
    count += mlist->method_count;
  }

  // List must be NULL terminated
  if (methods) {
    methods[count] = NULL;
  }
  if (outCount) *outCount = count;
  return methods;
}

SEL method_getName(Method method) {
  if (!method) return NULL;
  return method->method_name;
}

IMP method_getImplementation(Method method) {
  if (!method) return NULL;
  return method->method_imp;
}

IMP method_setImplementation(Method method, IMP imp) {
  // We intentionally don't test method for nil.
  // Leopard fails here, so should we.
  // I logged this as Radar:
  // 5572981 method_setImplementation crashes if you pass nil for the
  // method arg (arg 1)
  // because it seems odd that this API won't accept nil for method considering
  // all the other apis will accept nil args.
  // If this does get fixed, remember to enable the unit tests.
  // This method works differently on SnowLeopard than
  // on Leopard. If you pass in a nil for IMP on SnowLeopard
  // it doesn't change anything. On Leopard it will. Since
  // attempting to change a sel to nil is probably an error
  // we follow the SnowLeopard way of doing things.
  IMP oldImp = NULL;
  if (imp) {
    oldImp = method->method_imp;
    method->method_imp = imp;
  }
  return oldImp;
}

void method_exchangeImplementations(Method m1, Method m2) {
  if (m1 == m2) return;
  if (!m1 || !m2) return;
  IMP imp2 = method_getImplementation(m2);
  IMP imp1 = method_setImplementation(m1, imp2);
  method_setImplementation(m2, imp1);
}

struct objc_method_description protocol_getMethodDescription(Protocol *p,
                                                             SEL aSel,
                                                             BOOL isRequiredMethod,
                                                             BOOL isInstanceMethod) {
  struct objc_method_description *descPtr = NULL;
  // No such thing as required in ObjC1.
  if (isInstanceMethod) {
    descPtr = [p descriptionForInstanceMethod:aSel];
  } else {
    descPtr = [p descriptionForClassMethod:aSel];
  }

  struct objc_method_description desc;
  if (descPtr) {
    desc = *descPtr;
  } else {
    bzero(&desc, sizeof(desc));
  }
  return desc;
}

BOOL sel_isEqual(SEL lhs, SEL rhs) {
  // Apple (informally) promises this will work in the future:
  // http://twitter.com/#!/gparker/status/2400099786
  return (lhs == rhs) ? YES : NO;
}

#endif  // GTM_MACOS_SDK && (MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5)
