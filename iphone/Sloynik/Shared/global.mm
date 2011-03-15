//
//  global.mm
//  sloynik
//
//  Created by Yury Melnichek on 20.01.11.
//  Copyright 2011 -. All rights reserved.
//

#include "global.hpp"
#import "PerfCount.h"
#include "../../words/sloynik_engine.hpp"
#include "../../base/assert.hpp"

sl::StrFn::Str const * StrCreateApple(char const * utf8Str, uint32_t size)
{
  return reinterpret_cast<sl::StrFn::Str const *>(
      CFStringCreateWithBytes(kCFAllocatorDefault,
                              reinterpret_cast<UInt8 const *>(utf8Str),
                              size,
                              kCFStringEncodingUTF8,
                              false));
}

void StrDestroyApple(sl::StrFn::Str const * s)
{
  if (s)
    CFRelease(reinterpret_cast<CFStringRef>(s));
}

int StrPrimaryCompareApple(void *, sl::StrFn::Str const * a, sl::StrFn::Str const * b)
{
  CFStringRef sa = reinterpret_cast<CFStringRef>(a);
  CFStringRef sb = reinterpret_cast<CFStringRef>(b);
  return CFStringCompareWithOptionsAndLocale(sa, sb, CFRangeMake(0, CFStringGetLength(sa)),
                                             kCFCompareCaseInsensitive |
                                             kCFCompareNonliteral |
                                             kCFCompareWidthInsensitive |
                                             kCFCompareLocalized, NULL);
}

int StrSecondaryCompareApple(void *, sl::StrFn::Str const * a, sl::StrFn::Str const * b)
{
  CFStringRef sa = reinterpret_cast<CFStringRef>(a);
  CFStringRef sb = reinterpret_cast<CFStringRef>(b);
  return CFStringCompareWithOptionsAndLocale(sa, sb, CFRangeMake(0, CFStringGetLength(sa)),
                                             kCFCompareWidthInsensitive |
                                             kCFCompareLocalized, NULL);
}

sl::SloynikEngine * GetSloynikEngine()
{
  static sl::SloynikEngine * s_pEngine = NULL;
  if (!s_pEngine)
  {
    LogTimeCounter("EngineStartUp", "Starting engine.");
    NSBundle * bundle = [NSBundle mainBundle];
    string const dictionaryPath = [[bundle pathForResource:@"dictionary" ofType:@"slf"] UTF8String];
    string const indexPath = [[NSSearchPathForDirectoriesInDomains(NSCachesDirectory,
                                                                   NSUserDomainMask,
                                                                   YES)
                               objectAtIndex:0]
                              UTF8String] + string("/index");
    sl::StrFn strFn;
    strFn.Create = StrCreateApple;
    strFn.Destroy = StrDestroyApple;
    strFn.PrimaryCompare = StrPrimaryCompareApple;
    strFn.SecondaryCompare = StrSecondaryCompareApple;
    strFn.m_pData = NULL;
    strFn.m_PrimaryCompareId = 1;
    strFn.m_SecondaryCompareId = 2;
    s_pEngine = new sl::SloynikEngine(dictionaryPath, indexPath, strFn);
    LogTimeCounter("EngineStartUp", "Engine started.");
  }
  return s_pEngine;    
}
