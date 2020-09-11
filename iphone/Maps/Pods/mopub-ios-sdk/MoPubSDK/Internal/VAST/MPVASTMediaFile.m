//
//  MPVASTMediaFile.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTMediaFile.h"
#import "MPVASTStringUtilities.h"

static NSString * const kVideoMIMETypeMP4 = @"video/mp4";
static NSString * const kVideoMIMETypeQuickTime = @"video/quicktime";
static NSString * const kVideoMIMEType3GPP = @"video/3gpp";
static NSString * const kVideoMIMEType3GPP2 = @"video/3gpp2";
static NSString * const kVideoMIMETypeXM4V = @"video/x-m4v";

@implementation MPVASTMediaFile

+ (NSDictionary *)modelMap
{
    return @{@"bitrate":    @[@"bitrate", MPParseNumberFromString(NSNumberFormatterDecimalStyle)],
             @"height":     @[@"height", MPParseNumberFromString(NSNumberFormatterDecimalStyle)],
             @"width":      @[@"width", MPParseNumberFromString(NSNumberFormatterDecimalStyle)],
             @"identifier": @"id",
             @"delivery":   @"delivery",
             @"mimeType":   @"type",
             @"URL":        @[@"text", MPParseURLFromString()]};
}

@end

@implementation MPVASTMediaFile (Selection)

+ (MPVASTMediaFile *)bestMediaFileFromCandidates:(NSArray<MPVASTMediaFile *> *)candidates
                                forContainerSize:(CGSize)containerSize
                            containerScaleFactor:(CGFloat)containerScaleFactor {
    if (containerSize.width <= 0
        || containerSize.height <= 0
        || containerScaleFactor <= 0) {
        return nil;
    }

    CGFloat highestScore = CGFLOAT_MIN;
    MPVASTMediaFile *result;

    for (MPVASTMediaFile *candidate in candidates) {
        CGFloat score = [candidate selectionScoreForContainerSize:containerSize
                                             containerScaleFactor:containerScaleFactor];
        if (highestScore < score) {
            highestScore = score;
            result = candidate;
        }
    }

    return result;
}

#pragma mark - Private

- (CGFloat)formatScore {
    static dispatch_once_t onceToken;
    static NSDictionary<NSString *, NSNumber *> *scoreTable;
    dispatch_once(&onceToken, ^{
        scoreTable = @{kVideoMIMETypeMP4: @1.5,
                       kVideoMIMETypeQuickTime: @1,
                       kVideoMIMEType3GPP: @1,
                       kVideoMIMEType3GPP2: @1,
                       kVideoMIMETypeXM4V: @1};
    });
    return (CGFloat)scoreTable[self.mimeType].floatValue;
}

- (CGFloat)fitScoreForContainerSize:(CGSize)containerSize
               containerScaleFactor:(CGFloat)containerScaleFactor {
    if (self.width == 0 || self.height == 0) {
        return 0;
    }

    CGFloat aspectRatioScore = ABS(containerSize.width / containerSize.height
                                   - self.width / self.height);
    CGFloat widthScore = ABS((containerScaleFactor * containerSize.width - self.width)
                             / (containerScaleFactor * containerSize.width));
    return aspectRatioScore + widthScore;
}

- (CGFloat)qualityScore {
    const CGFloat lowBitrate = 700;
    const CGFloat highBitrate = 1500;
    if (lowBitrate <= self.bitrate && self.bitrate <= highBitrate) {
        return 0;
    } else {
        return MIN(ABS(lowBitrate - self.bitrate) / lowBitrate,
                   ABS(highBitrate - self.bitrate) / highBitrate);
    }
}

/**
 See scoring algorithm documentation at go/adf-vast-video-selection.
 */
- (CGFloat)selectionScoreForContainerSize:(CGSize)containerSize
                     containerScaleFactor:(CGFloat)containerScaleFactor {
    CGFloat fitScore = [self fitScoreForContainerSize:containerSize
                                 containerScaleFactor: containerScaleFactor];
    return self.formatScore / (1 + fitScore + self.qualityScore);
}

@end
