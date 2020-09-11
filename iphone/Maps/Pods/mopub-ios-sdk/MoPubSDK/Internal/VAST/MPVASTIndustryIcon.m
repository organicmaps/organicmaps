//
//  MPVASTIndustryIcon.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTIndustryIcon.h"
#import "MPVASTDurationOffset.h"
#import "MPVASTResource.h"
#import "MPVASTStringUtilities.h"

@interface MPVASTIndustryIcon ()

@property (nonatomic, readonly) MPVASTResource *HTMLResource;
@property (nonatomic, readonly) MPVASTResource *iframeResource;
@property (nonatomic, readonly) MPVASTResource *staticResource;

@end

@implementation MPVASTIndustryIcon

+ (NSDictionary *)modelMap
{
    return @{@"program":            @"program",
             @"height":             @[@"height", MPParseNumberFromString(NSNumberFormatterDecimalStyle)],
             @"width":              @[@"width", MPParseNumberFromString(NSNumberFormatterDecimalStyle)],
             @"xPosition":          @"xPosition",
             @"yPosition":          @"yPosition",
             @"clickThroughURL":    @[@"IconClicks.IconClickThrough.text", MPParseURLFromString()],
             @"clickTrackingURLs":  @[@"IconClicks.IconClickTracking.text", MPParseArrayOf(MPParseURLFromString())],
             @"viewTrackingURLs":   @[@"IconViewTracking.text", MPParseArrayOf(MPParseURLFromString())],
             @"apiFramework":       @"apiFramework",
             @"duration":           @[@"duration", MPParseTimeIntervalFromDurationString()],
             @"offset":             @[@"@self", MPParseClass([MPVASTDurationOffset class])],
             @"HTMLResource":       @[@"HTMLResource", MPParseClass([MPVASTResource class])],
             @"iframeResource":     @[@"IFrameResource", MPParseClass([MPVASTResource class])],
             @"staticResource":     @[@"StaticResource", MPParseClass([MPVASTResource class])]};
}

- (MPVASTResource *)resourceToDisplay {
    if (self.staticResource.content.length > 0) {
        if (self.staticResource.isStaticCreativeTypeImage) {
            self.staticResource.type = MPVASTResourceType_StaticImage;
            return self.staticResource;
        }
        if (self.staticResource.isStaticCreativeTypeJavaScript) {
            self.staticResource.type = MPVASTResourceType_StaticScript;
            return self.staticResource;
        }
    }

    if (self.HTMLResource.content.length > 0) {
        self.HTMLResource.type = MPVASTResourceType_HTML;
        return self.HTMLResource;
    }

    if (self.iframeResource.content.length > 0) {
        self.iframeResource.type = MPVASTResourceType_Iframe;
        return self.iframeResource;
    }

    return nil;
}

@end
