//
//  main.m
//  sloynik
//
//  Created by Yury Melnichek on 05.08.10.
//  Copyright - 2010. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "PerfCount.h"

int main(int argc, char *argv[])
{
  LogTimeCounter("StartTime", "main");        
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

  int retVal = UIApplicationMain(argc, argv, nil, @"AppDelegate");

  [pool release];
  return retVal;
}
