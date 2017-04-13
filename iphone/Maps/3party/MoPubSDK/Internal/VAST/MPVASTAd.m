//
//  MPVASTAd.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPVASTAd.h"
#import "MPVASTInline.h"
#import "MPVASTWrapper.h"
#import "MPLogging.h"

@implementation MPVASTAd

- (instancetype)initWithDictionary:(NSDictionary *)dictionary
{
    self = [super initWithDictionary:dictionary];
    if (self) {
        // The VAST spec (2.2.2.2) prohibits an <Ad> element from having both an <InLine> and a
        // <Wrapper> element. If both are present, we'll only allow the <InLine> element.
        if (_inlineAd && _wrapper) {
            MPLogWarn(@"VAST <Ad> element is not allowed to contain both an <InLine> and a "
                      @"<Wrapper>. The <Wrapper> will be ignored.");
            _wrapper = nil;
        }
    }
    return self;
}

+ (NSDictionary *)modelMap
{
    return @{@"identifier": @"id",
             @"sequence":   @"sequence",
             @"inlineAd":   @[@"InLine", MPParseClass([MPVASTInline class])],
             @"wrapper":    @[@"Wrapper", MPParseClass([MPVASTWrapper class])]};
}

@end
