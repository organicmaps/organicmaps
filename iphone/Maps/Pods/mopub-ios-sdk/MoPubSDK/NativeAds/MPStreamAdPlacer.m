//
//  MPStreamAdPlacer.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdPositioning.h"
#import "MPLogging.h"
#import "MPNativeAd+Internal.h"
#import "MPNativeAdData.h"
#import "MPNativeAdDelegate.h"
#import "MPNativeAdRendererConfiguration.h"
#import "MPNativeAdRendererConstants.h"
#import "MPNativeAdRendering.h"
#import "MPNativeAdSource.h"
#import "MPNativePositionSource.h"
#import "MPNativeView.h"
#import "MPServerAdPositioning.h"
#import "MPStaticNativeAdRenderer.h"
#import "MPStreamAdPlacementData.h"
#import "MPStreamAdPlacer.h"

static NSInteger const kAdInsertionLookAheadAmount = 3;
static const NSUInteger kIndexPathItemIndex = 1;

@protocol MPNativeAdRenderer;

@interface MPStreamAdPlacer () <MPNativeAdSourceDelegate, MPNativeAdDelegate>

@property (nonatomic, strong) NSArray *rendererConfigurations;
@property (nonatomic, strong) MPNativeAdSource *adSource;
@property (nonatomic, strong) MPNativePositionSource *positioningSource;
@property (nonatomic, copy) MPAdPositioning *adPositioning;
@property (nonatomic, strong) MPStreamAdPlacementData *adPlacementData;
@property (nonatomic, copy) NSString *adUnitID;
@property (nonatomic, strong) NSMutableDictionary *sectionCounts;
@property (nonatomic, strong) NSIndexPath *topConsideredIndexPath;
@property (nonatomic, strong) NSIndexPath *bottomConsideredIndexPath;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPStreamAdPlacer

+ (instancetype)placerWithViewController:(UIViewController *)controller adPositioning:(MPAdPositioning *)positioning rendererConfigurations:(NSArray *)rendererConfigurations
{
    MPStreamAdPlacer *placer = [[self alloc] initWithViewController:controller adPositioning:positioning rendererConfigurations:rendererConfigurations];
    return placer;
}

- (instancetype)initWithViewController:(UIViewController *)controller adPositioning:(MPAdPositioning *)positioning rendererConfigurations:(NSArray *)rendererConfigurations
{
    NSAssert(controller != nil, @"A stream ad placer cannot be instantiated with a nil view controller.");
    NSAssert(positioning != nil, @"A stream ad placer cannot be instantiated with a nil positioning object.");

    for (id rendererConfiguration in rendererConfigurations) {
        NSAssert([rendererConfiguration isKindOfClass:[MPNativeAdRendererConfiguration class]], @"A stream ad placer must be instantiated with rendererConfigurations that are of type MPNativeAdRendererConfiguration.");
    }

    self = [super init];
    if (self) {
        _viewController = controller;
        _adPositioning = [positioning copy];
        _adSource = [[MPNativeAdSource alloc] initWithDelegate:self];
        _adPlacementData = [[MPStreamAdPlacementData alloc] initWithPositioning:_adPositioning];
        _rendererConfigurations = rendererConfigurations;
        _sectionCounts = [[NSMutableDictionary alloc] init];
    }
    return self;
}

- (void)dealloc
{
    [_positioningSource cancel];
}

- (void)setVisibleIndexPaths:(NSArray *)visibleIndexPaths
{
    if (visibleIndexPaths.count == 0) {
        _visibleIndexPaths = nil;
        self.topConsideredIndexPath = nil;
        self.bottomConsideredIndexPath = nil;
        return;
    }

    _visibleIndexPaths = [visibleIndexPaths sortedArrayUsingSelector:@selector(compare:)];
    self.topConsideredIndexPath = self.visibleIndexPaths.firstObject;
    self.bottomConsideredIndexPath = [self furthestValidIndexPathAfterIndexPath:self.visibleIndexPaths.lastObject withinDistance:visibleIndexPaths.count + kAdInsertionLookAheadAmount];

    [self fillAdsInConsideredRange];
}

- (void)setItemCount:(NSUInteger)count forSection:(NSInteger)section
{
    self.sectionCounts[@(section)] = @(count);
}

- (void)renderAdAtIndexPath:(NSIndexPath *)indexPath inView:(UIView *)view
{
    MPNativeAdData *adData = [self.adPlacementData adDataAtAdjustedIndexPath:indexPath];

    if (!adData) {
        MPLogInfo(@"-renderAdAtIndexPath: An ad does not exist at indexPath");
        return;
    }

    // Remove any old native ad views from the view prior to adding the new ad view as a sub view.
    for (UIView *subview in view.subviews) {
        if ([subview isKindOfClass:[MPNativeView class]]) {
            [subview removeFromSuperview];
        }
    }

    [view addSubview:[adData.ad retrieveAdViewWithError:nil]];

    CGSize adSize = [self sizeForAd:adData.ad withMaximumWidth:view.bounds.size.width andIndexPath:indexPath];
    [adData.ad updateAdViewSize:adSize];
}

- (CGSize)sizeForAdAtIndexPath:(NSIndexPath *)indexPath withMaximumWidth:(CGFloat)maxWidth
{
    MPNativeAdData *adData = [self.adPlacementData adDataAtAdjustedIndexPath:indexPath];

    // Tell the ad that it should resize the native ad view.
    CGSize adSize = [self sizeForAd:adData.ad withMaximumWidth:maxWidth andIndexPath:indexPath];
    [adData.ad updateAdViewSize:adSize];

    return adSize;
}

- (void)loadAdsForAdUnitID:(NSString *)adUnitID
{
    [self loadAdsForAdUnitID:adUnitID targeting:nil];
}

- (void)loadAdsForAdUnitID:(NSString *)adUnitID targeting:(MPNativeAdRequestTargeting *)targeting
{
    self.adUnitID = adUnitID;

    // Gather all the index paths with ads so we can notify the delegate that ads were removed.
    NSMutableArray *adIndexPaths = [NSMutableArray array];
    for (NSNumber *section in self.sectionCounts) {
        NSInteger intSection = [section unsignedIntegerValue];
        [adIndexPaths addObjectsFromArray:[self.adPlacementData adjustedIndexPathsWithAdsInSection:intSection]];
    }

    if (!adUnitID) {
        // We need some placement data.  Pass nil to it so it doesn't do any unnecessary work.
        self.adPlacementData = [[MPStreamAdPlacementData alloc] initWithPositioning:nil];
    } else if ([self.adPositioning isKindOfClass:[MPClientAdPositioning class]]) {
        // Reset to a placement data that has "desired" ads but not "placed" ones.
        self.adPlacementData = [[MPStreamAdPlacementData alloc] initWithPositioning:self.adPositioning];
    } else if ([self.adPositioning isKindOfClass:[MPServerAdPositioning class]]) {
        // Reset to a placement data that has no "desired" ads at all.
        self.adPlacementData = [[MPStreamAdPlacementData alloc] initWithPositioning:nil];

        // Get positioning information from the server.
        self.positioningSource = [[MPNativePositionSource alloc] init];
        __weak __typeof__(self) weakSelf = self;
        [self.positioningSource loadPositionsWithAdUnitIdentifier:self.adUnitID completionHandler:^(MPAdPositioning *positioning, NSError *error) {
            __typeof__(self) strongSelf = weakSelf;

            if (!strongSelf) {
                return;
            }

            if (error) {
                if ([error code] == MPNativePositionSourceEmptyResponse) {
                    MPLogInfo(@"ERROR: Ad placer cannot show any ads because ad positions have "
                               @"not been configured for your ad unit %@. You must assign positions "
                               @"by editing the ad unit's settings on the MoPub website.",
                               strongSelf.adUnitID);
                } else {
                    MPLogInfo(@"ERROR: Ad placer failed to get positions from the ad server for "
                               @"ad unit ID %@. Error: %@", strongSelf.adUnitID, error);
                }
            } else {
                strongSelf.adPlacementData = [[MPStreamAdPlacementData alloc] initWithPositioning:positioning];
            }
        }];
    }

    if (adIndexPaths.count > 0) {
        [self.delegate adPlacer:self didRemoveAdsAtIndexPaths:adIndexPaths];
    }

    if (!adUnitID) {
        MPLogInfo(@"Ad placer cannot load ads with a nil ad unit ID.");
        return;
    }

    [self.adSource loadAdsWithAdUnitIdentifier:adUnitID rendererConfigurations:self.rendererConfigurations andTargeting:targeting];
}

- (BOOL)isAdAtIndexPath:(NSIndexPath *)indexPath
{
    return [self.adPlacementData isAdAtAdjustedIndexPath:indexPath];
}

- (NSUInteger)adjustedNumberOfItems:(NSUInteger)numberOfItems inSection:(NSUInteger)section
{
    return [self.adPlacementData adjustedNumberOfItems:numberOfItems inSection:section];
}

- (NSIndexPath *)adjustedIndexPathForOriginalIndexPath:(NSIndexPath *)indexPath
{
    return [self.adPlacementData adjustedIndexPathForOriginalIndexPath:indexPath];
}

- (NSIndexPath *)originalIndexPathForAdjustedIndexPath:(NSIndexPath *)indexPath
{
    return [self.adPlacementData originalIndexPathForAdjustedIndexPath:indexPath];
}

- (NSArray *)adjustedIndexPathsForOriginalIndexPaths:(NSArray *)indexPaths
{
    NSMutableArray *adjustedIndexPaths = [NSMutableArray array];
    for (NSIndexPath *indexPath in indexPaths) {
        [adjustedIndexPaths addObject:[self adjustedIndexPathForOriginalIndexPath:indexPath]];
    }
    return [adjustedIndexPaths copy];
}

- (NSArray *)originalIndexPathsForAdjustedIndexPaths:(NSArray *)indexPaths
{
    NSMutableArray *originalIndexPaths = [NSMutableArray array];
    for (NSIndexPath *indexPath in indexPaths) {
        NSIndexPath *originalIndexPath = [self originalIndexPathForAdjustedIndexPath:indexPath];
        if (originalIndexPath) {
            [originalIndexPaths addObject:originalIndexPath];
        }
    }
    return [originalIndexPaths copy];
}

- (void)insertItemsAtIndexPaths:(NSArray *)originalIndexPaths
{
    [self.adPlacementData insertItemsAtIndexPaths:originalIndexPaths];
    [originalIndexPaths enumerateObjectsUsingBlock:^(NSIndexPath *originalIndexPath, NSUInteger idx, BOOL *stop) {
        NSInteger section = originalIndexPath.section;
        [self setItemCount:[[self.sectionCounts objectForKey:@(section)] integerValue] + 1 forSection:section];
    }];
}

- (void)deleteItemsAtIndexPaths:(NSArray *)originalIndexPaths
{
    originalIndexPaths = [originalIndexPaths sortedArrayUsingSelector:@selector(compare:)];
    NSMutableSet *activeSections = [NSMutableSet setWithCapacity:[originalIndexPaths count]];
    [originalIndexPaths enumerateObjectsUsingBlock:^(NSIndexPath *indexPath, NSUInteger idx, BOOL *stop) {
        [activeSections addObject:[NSNumber numberWithInteger:indexPath.section]];
    }];

    NSMutableArray *removedIndexPaths = [NSMutableArray array];
    [activeSections enumerateObjectsUsingBlock:^(NSNumber *section, BOOL *stop) {
        NSArray *originalIndexPathsInSection = [originalIndexPaths filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"section = %@", section]];
        NSRange deleteRange = [self rangeToDeleteInSection:section forOriginalContentIndexPaths:originalIndexPathsInSection];

        NSArray *indexPathsToDelete = [self.adPlacementData adjustedAdIndexPathsInAdjustedRange:deleteRange inSection:[section integerValue]];
        [removedIndexPaths addObjectsFromArray:indexPathsToDelete];
        [self.adPlacementData clearAdsInAdjustedRange:deleteRange inSection:[section integerValue]];
    }];

    [self.adPlacementData deleteItemsAtIndexPaths:originalIndexPaths];

    [originalIndexPaths enumerateObjectsUsingBlock:^(NSIndexPath *originalIndexPath, NSUInteger idx, BOOL *stop) {
        NSInteger section = originalIndexPath.section;
        [self setItemCount:[[self.sectionCounts objectForKey:@(section)] integerValue] - 1 forSection:section];
    }];

    if ([removedIndexPaths count]) {
        [self.delegate adPlacer:self didRemoveAdsAtIndexPaths:removedIndexPaths];
    }
}

- (void)moveItemAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
{
    [self.adPlacementData moveItemAtIndexPath:fromIndexPath toIndexPath:toIndexPath];
}

- (void)insertSections:(NSIndexSet *)sections
{
    [self.adPlacementData insertSections:sections];
    [self insertItemCountSections:sections];
}

- (void)deleteSections:(NSIndexSet *)sections
{
    [self.adPlacementData deleteSections:sections];
    [self deleteItemCountSections:sections];
}

- (void)moveSection:(NSInteger)section toSection:(NSInteger)newSection
{
    [self.adPlacementData moveSection:section toSection:newSection];

    NSUInteger originalSectionCount = [self.sectionCounts[@(section)] unsignedIntegerValue];

    [self deleteItemCountSections:[NSIndexSet indexSetWithIndex:section]];
    [self insertItemCountSections:[NSIndexSet indexSetWithIndex:newSection]];

    [self setItemCount:originalSectionCount forSection:newSection];
}

#pragma mark - Private

- (void)insertItemCountSections:(NSIndexSet *)sections
{
    [sections enumerateIndexesUsingBlock:^(NSUInteger insertionSection, BOOL *stop) {
        // Explicitly casting indices to NSInteger because we're counting backwards.
        NSInteger maxSection = [[[self.sectionCounts allKeys] valueForKeyPath:@"@max.unsignedIntValue"] unsignedIntegerValue];
        NSInteger signedInsertionSection = insertionSection;

        // We need to shift all the data above the new section up by 1. This assumes each section has a count (as each section should).
        for (NSInteger i = maxSection; i >= signedInsertionSection; --i) {
            NSUInteger currentCount = [self.sectionCounts[@(i)] unsignedIntegerValue];
            [self setItemCount:currentCount forSection:i+1];
        }

        // Setting the count to 0 isn't exactly correct, but it will be updated correctly when numberOfItems/Rows is called onthe data source.
        [self setItemCount:0 forSection:insertionSection];
    }];
}

- (void)deleteItemCountSections:(NSIndexSet *)sections
{
    [sections enumerateIndexesWithOptions:NSEnumerationReverse usingBlock:^(NSUInteger deletionSection, BOOL *stop) {
        NSUInteger maxSection = [[[self.sectionCounts allKeys] valueForKeyPath:@"@max.unsignedIntValue"] unsignedIntegerValue];

        // We need to shift all the data above the deletionSection down by 1. This assumes each section has a count (as each section should).
        for (NSUInteger i = deletionSection; i <= maxSection; ++i) {
            NSUInteger nextCount = [self.sectionCounts[@(i+1)] unsignedIntegerValue];
            [self setItemCount:nextCount forSection:i];
        }

        [self.sectionCounts removeObjectForKey:@(maxSection)];
    }];
}

/*
 * Returns the range to consider removing cells from the datasource for a given section.
 *
 * We want to prevent a state where ads are present after the last remaining content item.
 * In order to do this, we need to find the range between the last remaining content item
 * (after deletion occurs) and last item being deleted. If the end of this range includes
 * the (current) last remaining item, we should delete all ads within the range, since they
 * would otherwise be "trailing" ads.
 *
 * The range returned from this method will not include any ads that appear before the
 * last remaining content item, because all we care about is preventing trailing ads.
 */
- (NSRange)rangeToDeleteInSection:(NSNumber *)section forOriginalContentIndexPaths:(NSArray *)originalContentIndexPaths
{
    NSRange rangeToDelete = NSMakeRange(0, 0);
    NSInteger sectionCount = [self.sectionCounts[section] integerValue];
    //In order to remove trailing ads, we need to find the first index path of the last contiguous block of content items
    //That we're deleting. Using the item of this index path, we can create a range in which to remove ads from the datasource.
    __block NSIndexPath *firstIndexPathOfLastContiguousContentItemBlock = [originalContentIndexPaths lastObject];

    //determines if the last content item is being deleted. If not, no ads will be deleted.
    if (sectionCount == firstIndexPathOfLastContiguousContentItemBlock.row + 1) {

        //Traverses (in reverse) the content index paths being deleted until it reaches the beginning of the section (all items being deleted), or a gap in that block.
        [[[originalContentIndexPaths reverseObjectEnumerator] allObjects] enumerateObjectsUsingBlock:^(NSIndexPath *contentPath, NSUInteger idx, BOOL *stop) {
            if (idx > 0 && contentPath.row == firstIndexPathOfLastContiguousContentItemBlock.row - 1) {
                firstIndexPathOfLastContiguousContentItemBlock = contentPath;
            }
        }];

        NSInteger sectionTotal = [self.adPlacementData adjustedNumberOfItems:sectionCount inSection:[section integerValue]];

        if (firstIndexPathOfLastContiguousContentItemBlock.row == 0) {
            rangeToDelete = NSMakeRange(0, sectionTotal);
        } else {
            //Last content item *not* being deleted - will be the new end of the section.
            NSIndexPath *lastRemainingContentIndexPath = [NSIndexPath indexPathForRow:firstIndexPathOfLastContiguousContentItemBlock.row - 1 inSection:firstIndexPathOfLastContiguousContentItemBlock.section];
            NSIndexPath *adjustedLastContent = [self.adPlacementData adjustedIndexPathForOriginalIndexPath:lastRemainingContentIndexPath];
            rangeToDelete = NSMakeRange(adjustedLastContent.row, sectionTotal - adjustedLastContent.row);
        }
    }
    return rangeToDelete;
}

- (NSIndexPath *)furthestValidIndexPathAfterIndexPath:(NSIndexPath *)startingPath withinDistance:(NSUInteger)numberOfItems
{
    NSUInteger section = [startingPath indexAtPosition:0];
    NSInteger itemIndex = [startingPath indexAtPosition:1];

    NSNumber *sectionCountNumber = self.sectionCounts[@(section)];
    NSUInteger sectionItemCount = [sectionCountNumber unsignedIntegerValue];
    NSUInteger itemsPassed = 0;
    while (itemsPassed < numberOfItems) {
        if (sectionItemCount > (itemIndex + 1)) {
            ++itemIndex;
            ++itemsPassed;
        } else {
            // Ignore 0 sized sections.
            NSUInteger trySection = section;
            do {
                ++trySection;
                sectionCountNumber = self.sectionCounts[@(trySection)];
                sectionItemCount = [sectionCountNumber unsignedIntegerValue];
            } while (sectionCountNumber && sectionItemCount == 0);

            // We can exit and use the last known valid index path if we can't get a section count number.
            if (!sectionCountNumber) {
                break;
            } else {
                // Otherwise we move onto the next non-zero section.
                section = trySection;
                ++itemsPassed;
                itemIndex = 0;
            }
        }
    }

    NSUInteger indices[] = {section, itemIndex};
    return [NSIndexPath indexPathWithIndexes:indices length:2];
}

- (NSIndexPath *)earliestValidIndexPathBeforeIndexPath:(NSIndexPath *)startingPath withinDistance:(NSUInteger)numberOfItems
{
    NSUInteger section = [startingPath indexAtPosition:0];
    NSInteger itemIndex = [startingPath indexAtPosition:1];

    NSUInteger itemsPassed = 0;
    while (itemsPassed < numberOfItems) {
        if ((itemIndex - 1) >= 0) {
            --itemIndex;
            ++itemsPassed;
        } else {
            // Ignore 0 sized sections.
            NSNumber *sectionCountNumber;
            NSUInteger trySection = section;
            NSUInteger trySectionCount;

            do {
                --trySection;
                sectionCountNumber = self.sectionCounts[@(trySection)];
                trySectionCount = [sectionCountNumber unsignedIntegerValue];
            } while (sectionCountNumber && trySectionCount == 0);

            // Exit and use the last known valid index path.
            if (!sectionCountNumber) {
                break;
            } else {
                // Otherwise we move backwards.
                section = trySection;
                itemIndex = trySectionCount - 1;
                ++itemsPassed;
            }
        }
    }

    NSUInteger indices[] = {section, itemIndex};
    return [NSIndexPath indexPathWithIndexes:indices length:2];
}

// Determines whether or not insertionPath is close enough to the visible cells to place an ad at insertionPath.
- (BOOL)shouldPlaceAdAtIndexPath:(NSIndexPath *)insertionPath
{
    if (!self.topConsideredIndexPath || !self.bottomConsideredIndexPath || !insertionPath) {
        return NO;
    }

    // We need to make sure the insertionPath is actually at a valid index in a section by confirming the index is less than the count in the section.
    NSUInteger originalSectionCount = [self.sectionCounts[@(insertionPath.section)] unsignedIntegerValue];
    NSUInteger adjustedSectionCount = [self adjustedNumberOfItems:originalSectionCount inSection:insertionPath.section];

    if ([insertionPath indexAtPosition:kIndexPathItemIndex] >= adjustedSectionCount) {
        return NO;
    }

    NSIndexPath *topAdjustedIndexPath = [self adjustedIndexPathForOriginalIndexPath:self.topConsideredIndexPath];
    NSIndexPath *bottomAdjustedIndexPath = [self adjustedIndexPathForOriginalIndexPath:self.bottomConsideredIndexPath];

    return ([topAdjustedIndexPath compare:insertionPath] != NSOrderedDescending) && ([bottomAdjustedIndexPath compare:insertionPath] != NSOrderedAscending);
}

- (MPNativeAdData *)retrieveAdDataForInsertionPath:(NSIndexPath *)insertionPath
{
    MPNativeAd *adObject = [self.adSource dequeueAdForAdUnitIdentifier:self.adUnitID];

    if (!adObject) {
        return nil;
    }

    MPNativeAdData *adData = [[MPNativeAdData alloc] init];
    adData.adUnitID = self.adUnitID;
    adData.ad = adObject;

    return adData;
}

- (void)fillAdsInConsideredRange
{
    if (!self.topConsideredIndexPath || !self.bottomConsideredIndexPath) {
        return;
    }

    NSIndexPath *topAdjustedIndexPath = [self adjustedIndexPathForOriginalIndexPath:self.topConsideredIndexPath];
    NSIndexPath *insertionPath = [self.adPlacementData nextAdInsertionIndexPathForAdjustedIndexPath:topAdjustedIndexPath];

    while ([self shouldPlaceAdAtIndexPath:insertionPath]) {
        MPNativeAdData *adData = [self retrieveAdDataForInsertionPath:insertionPath];
        adData.ad.delegate = self;

        if (!adData) {
            break;
        }

        [self.adPlacementData insertAdData:adData atIndexPath:insertionPath];
        [self.delegate adPlacer:self didLoadAdAtIndexPath:insertionPath];

        insertionPath = [self.adPlacementData nextAdInsertionIndexPathForAdjustedIndexPath:insertionPath];
    }
}

#pragma mark - <MPNativeAdSourceDelegate>

- (void)adSourceDidFinishRequest:(MPNativeAdSource *)source
{
    [self fillAdsInConsideredRange];
}

#pragma mark - <MPNativeAdDelegate>

- (UIViewController *)viewControllerForPresentingModalView
{
    return self.viewController;
}

- (void)willPresentModalForNativeAd:(MPNativeAd *)nativeAd
{
    if ([self.delegate respondsToSelector:@selector(nativeAdWillPresentModalForStreamAdPlacer:)]) {
        [self.delegate nativeAdWillPresentModalForStreamAdPlacer:self];
    }
}

- (void)didDismissModalForNativeAd:(MPNativeAd *)nativeAd
{
    if ([self.delegate respondsToSelector:@selector(nativeAdDidDismissModalForStreamAdPlacer:)]) {
        [self.delegate nativeAdDidDismissModalForStreamAdPlacer:self];
    }
}

- (void)willLeaveApplicationFromNativeAd:(MPNativeAd *)nativeAd
{
    if ([self.delegate respondsToSelector:@selector(nativeAdWillLeaveApplicationFromStreamAdPlacer:)]) {
        [self.delegate nativeAdWillLeaveApplicationFromStreamAdPlacer:self];
    }
}

- (void)mopubAd:(id<MPMoPubAd>)ad didTrackImpressionWithImpressionData:(MPImpressionData *)impressionData {
    if ([self.delegate respondsToSelector:@selector(mopubAdPlacer:didTrackImpressionForAd:withImpressionData:)]) {
        [self.delegate mopubAdPlacer:self
             didTrackImpressionForAd:ad
                  withImpressionData:impressionData];
    }
}

#pragma mark - Internal

- (CGSize)sizeForAd:(MPNativeAd *)ad withMaximumWidth:(CGFloat)maxWidth andIndexPath:(NSIndexPath *)indexPath
{
    id<MPNativeAdRenderer> renderer = ad.renderer;

    CGSize adSize;

    if ([renderer respondsToSelector:@selector(viewSizeHandler)] && renderer.viewSizeHandler) {
        adSize = [renderer viewSizeHandler](maxWidth);
        if (adSize.height == MPNativeViewDynamicDimension) {
            UIView *adView = [ad retrieveAdViewForSizeCalculationWithError:nil];
            if (adView) {
                CGSize hydratedAdViewSize = [adView sizeThatFits:CGSizeMake(adSize.width, CGFLOAT_MAX)];
                return hydratedAdViewSize;
            }
        }
        return adSize;
    }

    adSize = CGSizeMake(maxWidth, 44.0f);
    MPLogInfo(@"WARNING: + (CGSize)viewSizeHandler is NOT implemented for native ad renderer %@ at index path %@. You MUST implement this method to ensure that ad placer native ad cells are correctly sized. Returning a default size of %@ for now.", NSStringFromClass([(id)renderer class]), indexPath, NSStringFromCGSize(adSize));

    return adSize;
}
@end
