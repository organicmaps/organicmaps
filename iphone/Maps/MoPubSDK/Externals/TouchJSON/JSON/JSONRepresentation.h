//
//  JSONRepresentation.h
//  TouchJSON
//
//  Created by Jonathan Wight on 10/15/10.
//  Copyright 2010 toxicsoftware.com. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol JSONRepresentation

@optional
- (id)initWithJSONDataRepresentation:(NSData *)inJSONData;

- (NSData *)JSONDataRepresentation;

@end
