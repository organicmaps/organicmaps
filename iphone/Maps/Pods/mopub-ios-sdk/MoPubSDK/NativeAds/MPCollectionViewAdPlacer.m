//
//  MPCollectionViewAdPlacer.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPCollectionViewAdPlacer.h"
#import "MPStreamAdPlacer.h"
#import "MPAdPlacerInvocation.h"
#import "MPTimer.h"
#import "MPNativeAdUtils.h"
#import "MPCollectionViewAdPlacerCell.h"
#import "MPNativeAdRendererConfiguration.h"
#import <objc/runtime.h>

static NSString * const kCollectionViewAdPlacerReuseIdentifier = @"MPCollectionViewAdPlacerReuseIdentifier";

@protocol MPNativeAdRenderer;

@interface MPCollectionViewAdPlacer () <UICollectionViewDataSource, UICollectionViewDelegate, MPStreamAdPlacerDelegate, UICollectionViewDelegateFlowLayout>

@property (nonatomic, strong) MPStreamAdPlacer *streamAdPlacer;
@property (nonatomic, strong) UICollectionView *collectionView;
@property (nonatomic, weak) id<UICollectionViewDataSource> originalDataSource;
@property (nonatomic, weak) id<UICollectionViewDelegate> originalDelegate;
@property (nonatomic, strong) MPTimer *insertionTimer;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPCollectionViewAdPlacer

+ (instancetype)placerWithCollectionView:(UICollectionView *)collectionView viewController:(UIViewController *)controller rendererConfigurations:(NSArray *)rendererConfigurations
{
    return [[self class] placerWithCollectionView:collectionView viewController:controller adPositioning:[MPServerAdPositioning positioning] rendererConfigurations:rendererConfigurations];
}

+ (instancetype)placerWithCollectionView:(UICollectionView *)collectionView viewController:(UIViewController *)controller adPositioning:(MPAdPositioning *)positioning rendererConfigurations:(NSArray *)rendererConfigurations
{
    MPCollectionViewAdPlacer *collectionViewAdPlacer = [[MPCollectionViewAdPlacer alloc] initWithCollectionView:collectionView viewController:controller adPositioning:positioning rendererConfigurations:rendererConfigurations];
    return collectionViewAdPlacer;
}

- (instancetype)initWithCollectionView:(UICollectionView *)collectionView viewController:(UIViewController *)controller adPositioning:(MPAdPositioning *)positioning rendererConfigurations:(NSArray *)rendererConfigurations
{
    for (id rendererConfiguration in rendererConfigurations) {
        NSAssert([rendererConfiguration isKindOfClass:[MPNativeAdRendererConfiguration class]], @"A collection view ad placer must be instantiated with rendererConfigurations that are of type MPNativeAdRendererConfiguration.");
    }

    if (self = [super init]) {
        _collectionView = collectionView;
        _streamAdPlacer = [MPStreamAdPlacer placerWithViewController:controller adPositioning:positioning rendererConfigurations:rendererConfigurations];
        _streamAdPlacer.delegate = self;

        _insertionTimer = [MPTimer timerWithTimeInterval:kUpdateVisibleCellsInterval
                                                  target:self
                                                selector:@selector(updateVisibleCells)
                                                 repeats:YES
                                             runLoopMode:NSRunLoopCommonModes];
        [_insertionTimer scheduleNow];

        _originalDataSource = collectionView.dataSource;
        _originalDelegate = collectionView.delegate;
        collectionView.dataSource = self;
        collectionView.delegate = self;

        [_collectionView registerClass:[MPCollectionViewAdPlacerCell class] forCellWithReuseIdentifier:kCollectionViewAdPlacerReuseIdentifier];

        [collectionView mp_setAdPlacer:self];
    }

    return self;
}

- (void)dealloc
{
    [_insertionTimer invalidate];
}

#pragma mark - Public

- (void)loadAdsForAdUnitID:(NSString *)adUnitID
{
    [self.streamAdPlacer loadAdsForAdUnitID:adUnitID];
}

- (void)loadAdsForAdUnitID:(NSString *)adUnitID targeting:(MPNativeAdRequestTargeting *)targeting
{
    [self.streamAdPlacer loadAdsForAdUnitID:adUnitID targeting:targeting];
}

#pragma mark - Ad Insertion

- (void)updateVisibleCells
{
    NSArray *visiblePaths = self.collectionView.mp_indexPathsForVisibleItems;

    if ([visiblePaths count]) {
        [self.streamAdPlacer setVisibleIndexPaths:visiblePaths];
    }
}

#pragma mark - <MPStreamAdPlacerDelegate>

- (void)adPlacer:(MPStreamAdPlacer *)adPlacer didLoadAdAtIndexPath:(NSIndexPath *)indexPath
{
    NSInteger sectionCount = [self.collectionView.dataSource numberOfSectionsInCollectionView:self.collectionView];
    NSInteger rowCount = [self.collectionView.dataSource collectionView:self.collectionView numberOfItemsInSection:indexPath.section];
    if (indexPath.section >= sectionCount || indexPath.row >= rowCount) {
        return; // ignore out-of-range index path that cannot be added to the collection view
    }

    BOOL animationsWereEnabled = [UIView areAnimationsEnabled];
    //We only want to enable animations if the index path is before or within our visible cells
    BOOL animationsEnabled = ([(NSIndexPath *)[self.collectionView.indexPathsForVisibleItems lastObject] compare:indexPath] != NSOrderedAscending) && animationsWereEnabled;

    [UIView setAnimationsEnabled:animationsEnabled];

    [self.collectionView insertItemsAtIndexPaths:@[indexPath]];

    [UIView setAnimationsEnabled:animationsWereEnabled];
}

- (void)adPlacer:(MPStreamAdPlacer *)adPlacer didRemoveAdsAtIndexPaths:(NSArray *)indexPaths
{
    NSMutableArray<NSIndexPath *> *validIndexPaths = [NSMutableArray new];
    for (NSIndexPath *indexPath in indexPaths) {
        NSInteger sectionCount = [self.collectionView numberOfSections];
        NSInteger rowCount = [self.collectionView numberOfItemsInSection:indexPath.section];

        // ignore out-of-range index path that cannot be removed from the collection view
        if (indexPath.section < sectionCount && indexPath.row < rowCount) {
            [validIndexPaths addObject:indexPath];
        }
    }

    if (validIndexPaths.count == 0) {
        return;
    }

    BOOL animationsWereEnabled = [UIView areAnimationsEnabled];
    [UIView setAnimationsEnabled:NO];

    [self.collectionView performBatchUpdates:^{
        [self.collectionView deleteItemsAtIndexPaths:validIndexPaths];
    } completion:^(BOOL finished) {
        [UIView setAnimationsEnabled:animationsWereEnabled];
    }];
}

- (void)nativeAdWillPresentModalForStreamAdPlacer:(MPStreamAdPlacer *)adPlacer
{
    if ([self.delegate respondsToSelector:@selector(nativeAdWillPresentModalForCollectionViewAdPlacer:)]) {
        [self.delegate nativeAdWillPresentModalForCollectionViewAdPlacer:self];
    }
}

- (void)nativeAdDidDismissModalForStreamAdPlacer:(MPStreamAdPlacer *)adPlacer
{
    if ([self.delegate respondsToSelector:@selector(nativeAdDidDismissModalForCollectionViewAdPlacer:)]) {
        [self.delegate nativeAdDidDismissModalForCollectionViewAdPlacer:self];
    }
}

- (void)nativeAdWillLeaveApplicationFromStreamAdPlacer:(MPStreamAdPlacer *)adPlacer
{
    if ([self.delegate respondsToSelector:@selector(nativeAdWillLeaveApplicationFromCollectionViewAdPlacer:)]) {
        [self.delegate nativeAdWillLeaveApplicationFromCollectionViewAdPlacer:self];
    }
}

- (void)mopubAdPlacer:(id<MPMoPubAdPlacer>)adPlacer didTrackImpressionForAd:(id<MPMoPubAd>)ad withImpressionData:(MPImpressionData *)impressionData {
    if ([self.delegate respondsToSelector:@selector(mopubAdPlacer:didTrackImpressionForAd:withImpressionData:)]) {
        [self.delegate mopubAdPlacer:self
             didTrackImpressionForAd:ad
                  withImpressionData:impressionData];
    }
}

#pragma mark - <UICollectionViewDataSource>

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView
{
    if ([self.originalDataSource respondsToSelector:@selector(numberOfSectionsInCollectionView:)]) {
        return [self.originalDataSource numberOfSectionsInCollectionView:collectionView];
    }
    else {
        return 1;
    }
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    NSUInteger numberOfItems = [self.originalDataSource collectionView:collectionView numberOfItemsInSection:section];
    [self.streamAdPlacer setItemCount:numberOfItems forSection:section];
    return [self.streamAdPlacer adjustedNumberOfItems:numberOfItems inSection:section];
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        MPCollectionViewAdPlacerCell *cell = (MPCollectionViewAdPlacerCell *)[collectionView dequeueReusableCellWithReuseIdentifier:kCollectionViewAdPlacerReuseIdentifier forIndexPath:indexPath];
        cell.clipsToBounds = YES;

        [self.streamAdPlacer renderAdAtIndexPath:indexPath inView:cell.contentView];
        return cell;
    }

    NSIndexPath *originalIndexPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
    return [self.originalDataSource collectionView:collectionView cellForItemAtIndexPath:originalIndexPath];
}

#pragma mark - <UICollectionViewDelegate>

- (BOOL)collectionView:(UICollectionView *)collectionView canPerformAction:(SEL)action forItemAtIndexPath:(NSIndexPath *)indexPath withSender:(id)sender
{
    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        return NO;
    }

    id<UICollectionViewDelegate> delegate = self.originalDelegate;
    if ([delegate respondsToSelector:@selector(collectionView:canPerformAction:forItemAtIndexPath:withSender:)]) {
        NSIndexPath *originalPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
        return [delegate collectionView:collectionView canPerformAction:action forItemAtIndexPath:originalPath withSender:sender];
    }

    return NO;
}

- (void)collectionView:(UICollectionView *)collectionView didDeselectItemAtIndexPath:(NSIndexPath *)indexPath
{
    [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(collectionView:didDeselectItemAtIndexPath:) firstArg:collectionView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];
}

- (void)collectionView:(UICollectionView *)collectionView didEndDisplayingCell:(UICollectionViewCell *)cell forItemAtIndexPath:(NSIndexPath *)indexPath
{
    [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with3ArgSelector:@selector(collectionView:didEndDisplayingCell:forItemAtIndexPath:) firstArg:collectionView secondArg:cell thirdArg:indexPath streamAdPlacer:self.streamAdPlacer];
}

- (void)collectionView:(UICollectionView *)collectionView didHighlightItemAtIndexPath:(NSIndexPath *)indexPath
{
    [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(collectionView:didHighlightItemAtIndexPath:) firstArg:collectionView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        // The view inside the cell already has a gesture recognizer to handle the tap event.
        [self.collectionView deselectItemAtIndexPath:indexPath animated:NO];
        return;
    }

    id<UICollectionViewDelegate> delegate = self.originalDelegate;
    if ([delegate respondsToSelector:@selector(collectionView:didSelectItemAtIndexPath:)]) {
        NSIndexPath *originalPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
        [delegate collectionView:collectionView didSelectItemAtIndexPath:originalPath];
    }
}

- (void)collectionView:(UICollectionView *)collectionView didUnhighlightItemAtIndexPath:(NSIndexPath *)indexPath
{
    [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(collectionView:didUnhighlightItemAtIndexPath:) firstArg:collectionView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];
}

- (void)collectionView:(UICollectionView *)collectionView performAction:(SEL)action forItemAtIndexPath:(NSIndexPath *)indexPath withSender:(id)sender
{
    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        return;
    }

    id<UICollectionViewDelegate> delegate = self.originalDelegate;
    if ([delegate respondsToSelector:@selector(collectionView:performAction:forItemAtIndexPath:withSender:)]) {
        NSIndexPath *originalPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
        [delegate collectionView:collectionView performAction:action forItemAtIndexPath:originalPath withSender:sender];
    }
}

- (BOOL)collectionView:(UICollectionView *)collectionView shouldDeselectItemAtIndexPath:(NSIndexPath *)indexPath
{
    NSInvocation *invocation = [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(collectionView:shouldDeselectItemAtIndexPath:) firstArg:collectionView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];

    return [MPAdPlacerInvocation boolResultForInvocation:invocation defaultValue:YES];
}

- (BOOL)collectionView:(UICollectionView *)collectionView shouldHighlightItemAtIndexPath:(NSIndexPath *)indexPath
{
    NSInvocation *invocation = [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(collectionView:shouldHighlightItemAtIndexPath:) firstArg:collectionView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];

    return [MPAdPlacerInvocation boolResultForInvocation:invocation defaultValue:YES];
}

- (BOOL)collectionView:(UICollectionView *)collectionView shouldSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    NSInvocation *invocation = [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(collectionView:shouldSelectItemAtIndexPath:) firstArg:collectionView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];

    return [MPAdPlacerInvocation boolResultForInvocation:invocation defaultValue:collectionView.allowsSelection];
}

- (BOOL)collectionView:(UICollectionView *)collectionView shouldShowMenuForItemAtIndexPath:(NSIndexPath *)indexPath
{
    NSInvocation *invocation = [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(collectionView:shouldShowMenuForItemAtIndexPath:) firstArg:collectionView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];

    return [MPAdPlacerInvocation boolResultForInvocation:invocation defaultValue:NO];
}

- (void)collectionView:(UICollectionView *)collectionView willDisplayCell:(UICollectionViewCell *)cell forItemAtIndexPath:(NSIndexPath *)indexPath
{
    if ([self.originalDelegate respondsToSelector:@selector(collectionView:willDisplayCell:forItemAtIndexPath:)]) {
        [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with3ArgSelector:@selector(collectionView:willDisplayCell:forItemAtIndexPath:) firstArg:collectionView secondArg:cell thirdArg:indexPath streamAdPlacer:self.streamAdPlacer];
    }
}

#pragma mark - UICollectionViewDelegateFlowLayout

- (CGSize)collectionView:(UICollectionView *)collectionView layout:(UICollectionViewLayout *)collectionViewLayout sizeForItemAtIndexPath:(NSIndexPath *)indexPath
{
    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        return [self.streamAdPlacer sizeForAdAtIndexPath:indexPath withMaximumWidth:CGRectGetWidth(self.collectionView.bounds)];
    }

    if ([self.originalDelegate respondsToSelector:@selector(collectionView:layout:sizeForItemAtIndexPath:)]) {
        NSIndexPath *originalPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
        id<UICollectionViewDelegateFlowLayout> flowLayout = (id<UICollectionViewDelegateFlowLayout>)[self originalDelegate];
        return [flowLayout collectionView:collectionView layout:collectionViewLayout sizeForItemAtIndexPath:originalPath];
    }
    return ((UICollectionViewFlowLayout *)collectionViewLayout).itemSize;
}

#pragma mark - Method Forwarding

- (BOOL)isKindOfClass:(Class)aClass {
    return [super isKindOfClass:aClass] ||
    [self.originalDataSource isKindOfClass:aClass] ||
    [self.originalDelegate isKindOfClass:aClass];
}

- (BOOL)conformsToProtocol:(Protocol *)aProtocol
{
    return [super conformsToProtocol:aProtocol] ||
    [self.originalDelegate conformsToProtocol:aProtocol] ||
    [self.originalDataSource conformsToProtocol:aProtocol];
}

- (BOOL)respondsToSelector:(SEL)aSelector
{
    return [super respondsToSelector:aSelector] ||
    [self.originalDataSource respondsToSelector:aSelector] ||
    [self.originalDelegate respondsToSelector:aSelector];
}

- (id)forwardingTargetForSelector:(SEL)aSelector
{
    if ([self.originalDataSource respondsToSelector:aSelector]) {
        return self.originalDataSource;
    } else if ([self.originalDelegate respondsToSelector:aSelector]) {
        return self.originalDelegate;
    } else {
        return [super forwardingTargetForSelector:aSelector];
    }
}

@end

@implementation UICollectionView (MPCollectionViewAdPlacer)

static char kAdPlacerKey;

- (void)mp_setAdPlacer:(MPCollectionViewAdPlacer *)placer
{
    objc_setAssociatedObject(self, &kAdPlacerKey, placer, OBJC_ASSOCIATION_ASSIGN);
}

- (MPCollectionViewAdPlacer *)mp_adPlacer
{
    return objc_getAssociatedObject(self, &kAdPlacerKey);
}

- (void)mp_setDelegate:(id<UICollectionViewDelegate>)delegate
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        adPlacer.originalDelegate = delegate;
    } else {
        self.delegate = delegate;
    }
}

- (id<UICollectionViewDelegate>)mp_delegate
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        return adPlacer.originalDelegate;
    } else {
        return self.delegate;
    }
}

- (void)mp_setDataSource:(id<UICollectionViewDataSource>)dataSource
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        adPlacer.originalDataSource = dataSource;
    } else {
        self.dataSource = dataSource;
    }
}

- (id<UICollectionViewDataSource>)mp_dataSource
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        return adPlacer.originalDataSource;
    } else {
        return self.dataSource;
    }
}

- (id)mp_dequeueReusableCellWithReuseIdentifier:(NSString *)identifier forIndexPath:(NSIndexPath *)indexPath
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = indexPath;

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:indexPath];
    }

    // Only pass nil through if developer passed it through
    if (!indexPath || adjustedIndexPath) {
        return [self dequeueReusableCellWithReuseIdentifier:identifier forIndexPath:adjustedIndexPath];
    } else {
        return nil;
    }
}

- (NSArray *)mp_indexPathsForSelectedItems
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSArray *adjustedIndexPaths = [self indexPathsForSelectedItems];

    if (adPlacer) {
        adjustedIndexPaths = [adPlacer.streamAdPlacer originalIndexPathsForAdjustedIndexPaths:adjustedIndexPaths];
    }

    return adjustedIndexPaths;
}

- (void)mp_selectItemAtIndexPath:(NSIndexPath *)indexPath animated:(BOOL)animated scrollPosition:(UICollectionViewScrollPosition)scrollPosition
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = indexPath;

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:indexPath];
    }

    // Only pass nil through if developer passed it through
    if (!indexPath || adjustedIndexPath) {
        [self selectItemAtIndexPath:adjustedIndexPath animated:animated scrollPosition:scrollPosition];
    }
}

- (void)mp_deselectItemAtIndexPath:(NSIndexPath *)indexPath animated:(BOOL)animated
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = indexPath;

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:indexPath];
    }

    [self deselectItemAtIndexPath:adjustedIndexPath animated:animated];
}

- (void)mp_reloadData
{
    [self reloadData];
}

- (UICollectionViewLayoutAttributes *)mp_layoutAttributesForItemAtIndexPath:(NSIndexPath *)indexPath
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = indexPath;

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:indexPath];
    }

    return [self layoutAttributesForItemAtIndexPath:adjustedIndexPath];
}

- (NSIndexPath *)mp_indexPathForItemAtPoint:(CGPoint)point
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = [self indexPathForItemAtPoint:point];

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer originalIndexPathForAdjustedIndexPath:adjustedIndexPath];
    }

    return adjustedIndexPath;
}

- (NSIndexPath *)mp_indexPathForCell:(UICollectionViewCell *)cell
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = [self indexPathForCell:cell];

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer originalIndexPathForAdjustedIndexPath:adjustedIndexPath];
    }

    return adjustedIndexPath;
}

- (UICollectionViewCell *)mp_cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = indexPath;

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:adjustedIndexPath];
    }

    return [self cellForItemAtIndexPath:adjustedIndexPath];
}

- (NSArray *)mp_visibleCells
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        NSArray *indexPaths = [self mp_indexPathsForVisibleItems];
        NSMutableArray *visibleCells = [NSMutableArray array];
        for (NSIndexPath *indexPath in indexPaths) {
            UICollectionViewCell * cell = [self mp_cellForItemAtIndexPath:indexPath];
            if (cell != nil) {
                [visibleCells addObject:cell];
            }
        }
        return visibleCells;
    } else {
        return [self visibleCells];
    }
}

- (NSArray *)mp_indexPathsForVisibleItems
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSArray *adjustedIndexPaths = [self indexPathsForVisibleItems];

    if (adPlacer) {
        adjustedIndexPaths = [adPlacer.streamAdPlacer originalIndexPathsForAdjustedIndexPaths:adjustedIndexPaths];
    }

    return adjustedIndexPaths;
}

- (void)mp_scrollToItemAtIndexPath:(NSIndexPath *)indexPath atScrollPosition:(UICollectionViewScrollPosition)scrollPosition animated:(BOOL)animated
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = indexPath;

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:adjustedIndexPath];
    }

    // Only pass nil through if developer passed it through
    if (!indexPath || adjustedIndexPath) {
        [self scrollToItemAtIndexPath:adjustedIndexPath atScrollPosition:scrollPosition animated:animated];
    }
}

- (void)mp_insertSections:(NSIndexSet *)sections
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        [adPlacer.streamAdPlacer insertSections:sections];
    }

    [self insertSections:sections];
}

- (void)mp_deleteSections:(NSIndexSet *)sections
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        [adPlacer.streamAdPlacer deleteSections:sections];
    }

    [self deleteSections:sections];
}

- (void)mp_reloadSections:(NSIndexSet *)sections
{
    [self reloadSections:sections];
}

- (void)mp_moveSection:(NSInteger)section toSection:(NSInteger)newSection
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        [adPlacer.streamAdPlacer moveSection:section toSection:newSection];
    }

    [self moveSection:section toSection:newSection];
}

- (void)mp_insertItemsAtIndexPaths:(NSArray *)indexPaths
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSArray *adjustedIndexPaths = indexPaths;

    if (adPlacer) {
        [adPlacer.streamAdPlacer insertItemsAtIndexPaths:indexPaths];
        adjustedIndexPaths = [adPlacer.streamAdPlacer adjustedIndexPathsForOriginalIndexPaths:indexPaths];
    }

    // We perform the actual UI insertion AFTER updating the stream ad placer's
    // data, because the insertion can trigger queries to the data source, which
    // needs to reflect the post-insertion state.
    [self insertItemsAtIndexPaths:adjustedIndexPaths];
}

- (void)mp_deleteItemsAtIndexPaths:(NSArray *)indexPaths
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    [self performBatchUpdates:^{
        NSArray *adjustedIndexPaths = indexPaths;

        if (adPlacer) {
            // We need to obtain the adjusted index paths to delete BEFORE we
            // update the stream ad placer's data.
            adjustedIndexPaths = [adPlacer.streamAdPlacer adjustedIndexPathsForOriginalIndexPaths:indexPaths];

            [adPlacer.streamAdPlacer deleteItemsAtIndexPaths:indexPaths];
        }

        // We perform the actual UI deletion AFTER updating the stream ad placer's
        // data, because the deletion can trigger queries to the data source, which
        // needs to reflect the post-deletion state.
        [self deleteItemsAtIndexPaths:adjustedIndexPaths];
    } completion:nil];
}

- (void)mp_reloadItemsAtIndexPaths:(NSArray *)indexPaths
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSArray *adjustedIndexPaths = indexPaths;

    if (adPlacer) {
        adjustedIndexPaths = [adPlacer.streamAdPlacer adjustedIndexPathsForOriginalIndexPaths:indexPaths];
    }

    [self reloadItemsAtIndexPaths:adjustedIndexPaths];
}

- (void)mp_moveItemAtIndexPath:(NSIndexPath *)indexPath toIndexPath:(NSIndexPath *)newIndexPath
{
    MPCollectionViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedFrom = indexPath;
    NSIndexPath *adjustedTo = newIndexPath;

    if (adPlacer) {
        // We need to obtain the adjusted index paths to move BEFORE we
        // update the stream ad placer's data.
        adjustedFrom = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:indexPath];
        adjustedTo = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:newIndexPath];

        [adPlacer.streamAdPlacer moveItemAtIndexPath:indexPath toIndexPath:newIndexPath];
    }

    // We perform the actual UI operation AFTER updating the stream ad placer's
    // data, because the operation can trigger queries to the data source, which
    // needs to reflect the post-operation state.
    [self moveItemAtIndexPath:adjustedFrom toIndexPath:adjustedTo];
}
@end
