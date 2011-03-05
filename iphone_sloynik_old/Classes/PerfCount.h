//
//  PerfCount.h
//  Sloynik
//
//  Created by Yury Melnichek on 10.09.09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>

#if __cplusplus
extern "C" {
#endif
	
void LogTimeCounter(const char * counter, const char * message);

#if __cplusplus
} // extern C
#endif