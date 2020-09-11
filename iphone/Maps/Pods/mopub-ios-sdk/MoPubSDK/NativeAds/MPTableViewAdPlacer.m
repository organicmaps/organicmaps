//
//  MPTableViewAdPlacer.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPTableViewAdPlacer.h"
#import "MPStreamAdPlacer.h"
#import "MPAdPlacerInvocation.h"
#import "MPTimer.h"
#import "MPNativeAdRendering.h"
#import "MPNativeAdUtils.h"
#import "MPGlobal.h"
#import "MPNativeAdRendererConfiguration.h"
#import "MPTableViewAdPlacerCell.h"
#import <objc/runtime.h>

static NSString * const kTableViewAdPlacerReuseIdentifier = @"MPTableViewAdPlacerReuseIdentifier";

@interface MPTableViewAdPlacer () <UITableViewDataSource, UITableViewDelegate, MPStreamAdPlacerDelegate>

@property (nonatomic, strong) MPStreamAdPlacer *streamAdPlacer;
@property (nonatomic, strong) UITableView *tableView;
@property (nonatomic, weak) id<UITableViewDataSource> originalDataSource;
@property (nonatomic, weak) id<UITableViewDelegate> originalDelegate;
@property (nonatomic, strong) MPTimer *insertionTimer;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPTableViewAdPlacer

+ (instancetype)placerWithTableView:(UITableView *)tableView viewController:(UIViewController *)controller rendererConfigurations:(NSArray *)rendererConfigurations
{
    return [[self class] placerWithTableView:tableView viewController:controller adPositioning:[MPServerAdPositioning positioning] rendererConfigurations:rendererConfigurations];
}

+ (instancetype)placerWithTableView:(UITableView *)tableView viewController:(UIViewController *)controller adPositioning:(MPAdPositioning *)positioning rendererConfigurations:(NSArray *)rendererConfigurations
{
    MPTableViewAdPlacer *tableViewAdPlacer = [[MPTableViewAdPlacer alloc] initWithTableView:tableView viewController:controller adPositioning:positioning rendererConfigurations:rendererConfigurations];
    return tableViewAdPlacer;
}

- (instancetype)initWithTableView:(UITableView *)tableView viewController:(UIViewController *)controller adPositioning:(MPAdPositioning *)positioning rendererConfigurations:(NSArray *)rendererConfigurations
{
    for (id rendererConfiguration in rendererConfigurations) {
        NSAssert([rendererConfiguration isKindOfClass:[MPNativeAdRendererConfiguration class]], @"A table view ad placer must be instantiated with rendererConfigurations that are of type MPNativeAdRendererConfiguration.");
    }

    if (self = [super init]) {
        _tableView = tableView;
        _streamAdPlacer = [MPStreamAdPlacer placerWithViewController:controller adPositioning:positioning rendererConfigurations:rendererConfigurations];
        _streamAdPlacer.delegate = self;

        _originalDataSource = tableView.dataSource;
        _originalDelegate = tableView.delegate;
        tableView.dataSource = self;
        tableView.delegate = self;

        [self.tableView registerClass:[MPTableViewAdPlacerCell class] forCellReuseIdentifier:kTableViewAdPlacerReuseIdentifier];

        [tableView mp_setAdPlacer:self];
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
    [self loadAdsForAdUnitID:adUnitID targeting:nil];
}

- (void)loadAdsForAdUnitID:(NSString *)adUnitID targeting:(MPNativeAdRequestTargeting *)targeting
{
    if (!self.insertionTimer) {
        self.insertionTimer = [MPTimer timerWithTimeInterval:kUpdateVisibleCellsInterval
                                                      target:self
                                                    selector:@selector(updateVisibleCells)
                                                     repeats:YES
                                                 runLoopMode:NSRunLoopCommonModes];
        [self.insertionTimer scheduleNow];
    }
    [self.streamAdPlacer loadAdsForAdUnitID:adUnitID targeting:targeting];
}

#pragma mark - Ad Insertion

- (void)updateVisibleCells
{
    NSArray *visiblePaths = self.tableView.mp_indexPathsForVisibleRows;

    if ([visiblePaths count]) {
        [self.streamAdPlacer setVisibleIndexPaths:visiblePaths];
    }
}

#pragma mark - <MPStreamAdPlacerDelegate>

- (void)adPlacer:(MPStreamAdPlacer *)adPlacer didLoadAdAtIndexPath:(NSIndexPath *)indexPath
{
    NSInteger sectionCount = [self.tableView.dataSource numberOfSectionsInTableView:self.tableView];
    NSInteger rowCount = [self.tableView.dataSource tableView:self.tableView numberOfRowsInSection:indexPath.section];
    if (indexPath.section >= sectionCount || indexPath.row >= rowCount) {
        return; // ignore out-of-range index path that cannot be added to the collection view
    }

    BOOL originalAnimationsEnabled = [UIView areAnimationsEnabled];
    //We only want to enable animations if the index path is before or within our visible cells
    BOOL animationsEnabled = ([(NSIndexPath *)[self.tableView.indexPathsForVisibleRows lastObject] compare:indexPath] != NSOrderedAscending) && originalAnimationsEnabled;

    [UIView setAnimationsEnabled:animationsEnabled];
    [self.tableView mp_beginUpdates];
    [self.tableView insertRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationMiddle];
    [self.tableView mp_endUpdates];
    [UIView setAnimationsEnabled:originalAnimationsEnabled];
}

- (void)adPlacer:(MPStreamAdPlacer *)adPlacer didRemoveAdsAtIndexPaths:(NSArray *)indexPaths
{
    NSMutableArray<NSIndexPath *> *validIndexPaths = [NSMutableArray new];
    for (NSIndexPath *indexPath in indexPaths) {
        NSInteger sectionCount = [self.tableView numberOfSections];
        NSInteger rowCount = [self.tableView numberOfRowsInSection:indexPath.section];

        // ignore out-of-range index path that cannot be removed from the table view
        if (indexPath.section < sectionCount && indexPath.row < rowCount) {
            [validIndexPaths addObject:indexPath];
        }
    }

    if (validIndexPaths.count == 0) {
        return;
    }

    BOOL originalAnimationsEnabled = [UIView areAnimationsEnabled];
    [UIView setAnimationsEnabled:NO];
    [self.tableView mp_beginUpdates];
    [self.tableView deleteRowsAtIndexPaths:validIndexPaths withRowAnimation:UITableViewRowAnimationNone];
    [self.tableView mp_endUpdates];
    [UIView setAnimationsEnabled:originalAnimationsEnabled];
}

- (void)nativeAdWillPresentModalForStreamAdPlacer:(MPStreamAdPlacer *)adPlacer
{
    if ([self.delegate respondsToSelector:@selector(nativeAdWillPresentModalForTableViewAdPlacer:)]) {
        [self.delegate nativeAdWillPresentModalForTableViewAdPlacer:self];
    }
}

- (void)nativeAdDidDismissModalForStreamAdPlacer:(MPStreamAdPlacer *)adPlacer
{
    if ([self.delegate respondsToSelector:@selector(nativeAdDidDismissModalForTableViewAdPlacer:)]) {
        [self.delegate nativeAdDidDismissModalForTableViewAdPlacer:self];
    }
}

- (void)nativeAdWillLeaveApplicationFromStreamAdPlacer:(MPStreamAdPlacer *)adPlacer
{
    if ([self.delegate respondsToSelector:@selector(nativeAdWillLeaveApplicationFromTableViewAdPlacer:)]) {
        [self.delegate nativeAdWillLeaveApplicationFromTableViewAdPlacer:self];
    }
}

- (void)mopubAdPlacer:(id<MPMoPubAdPlacer>)adPlacer didTrackImpressionForAd:(id<MPMoPubAd>)ad withImpressionData:(MPImpressionData *)impressionData {
    if ([self.delegate respondsToSelector:@selector(mopubAdPlacer:didTrackImpressionForAd:withImpressionData:)]) {
        [self.delegate mopubAdPlacer:self
             didTrackImpressionForAd:ad
                  withImpressionData:impressionData];
    }
}

#pragma mark - <UITableViewDataSource>

// Default is 1 if not implemented
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    if ([self.originalDataSource respondsToSelector:@selector(numberOfSectionsInTableView:)]) {
        return [self.originalDataSource numberOfSectionsInTableView:tableView];
    }
    else {
        return 1;
    }
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    NSUInteger numberOfItems = [self.originalDataSource tableView:tableView numberOfRowsInSection:section];
    [self.streamAdPlacer setItemCount:numberOfItems forSection:section];
    return [self.streamAdPlacer adjustedNumberOfItems:numberOfItems inSection:section];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        MPTableViewAdPlacerCell *cell = (MPTableViewAdPlacerCell *)[tableView dequeueReusableCellWithIdentifier:kTableViewAdPlacerReuseIdentifier forIndexPath:indexPath];
        cell.clipsToBounds = YES;

        [self.streamAdPlacer renderAdAtIndexPath:indexPath inView:cell.contentView];
        return cell;
    }
    NSIndexPath *originalIndexPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
    return [self.originalDataSource tableView:tableView cellForRowAtIndexPath:originalIndexPath];
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        return NO;
    }

    id<UITableViewDataSource> datasource = self.originalDataSource;
    if ([datasource respondsToSelector:@selector(tableView:canEditRowAtIndexPath:)]) {
        NSIndexPath *origPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
        return [datasource tableView:tableView canEditRowAtIndexPath:origPath];
    }

    // When the data source doesn't implement tableView:canEditRowAtIndexPath:, Apple assumes the cells are editable.  So we return YES.
    return YES;
}

- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSInvocation *invocation = [MPAdPlacerInvocation invokeForTarget:self.originalDataSource with2ArgSelector:@selector(tableView:canMoveRowAtIndexPath:) firstArg:tableView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];

    return [MPAdPlacerInvocation boolResultForInvocation:invocation defaultValue:NO];
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    [MPAdPlacerInvocation invokeForTarget:self.originalDataSource with3ArgIntSelector:@selector(tableView:commitEditingStyle:forRowAtIndexPath:) firstArg:tableView secondArg:editingStyle thirdArg:indexPath streamAdPlacer:self.streamAdPlacer];
}

- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)sourceIndexPath toIndexPath:(NSIndexPath *)destinationIndexPath
{
    if ([self.streamAdPlacer isAdAtIndexPath:sourceIndexPath]) {
        // Can't move an ad explicitly.
        return;
    }

    id<UITableViewDataSource> dataSource = self.originalDataSource;
    if ([dataSource respondsToSelector:@selector(tableView:moveRowAtIndexPath:toIndexPath:)]) {
        NSIndexPath *origSource = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:sourceIndexPath];
        NSIndexPath *origDestination = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:destinationIndexPath];
        [dataSource tableView:tableView moveRowAtIndexPath:origSource toIndexPath:origDestination];
    }
}

#pragma mark - <UITableViewDelegate>

// We don't override the following:
//
// -tableView:targetIndexPathForMoveFromRowAtIndexPath:toProposedIndexPath - No need to override because
// targeting is typically based on the adjusted paths.
//
// -tableView:accessoryTypeForRowWithIndexPath - Deprecated, and causes a runtime exception.

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        return [self.streamAdPlacer sizeForAdAtIndexPath:indexPath withMaximumWidth:CGRectGetWidth(self.tableView.bounds)].height;
    }

    if ([self.originalDelegate respondsToSelector:@selector(tableView:heightForRowAtIndexPath:)]) {
        NSIndexPath *originalIndexPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
        return [self.originalDelegate tableView:tableView heightForRowAtIndexPath:originalIndexPath];
    }

    return tableView.rowHeight;
}

- (void)tableView:(UITableView *)tableView willDisplayCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath
{
    [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with3ArgSelector:@selector(tableView:willDisplayCell:forRowAtIndexPath:) firstArg:tableView secondArg:cell thirdArg:indexPath streamAdPlacer:self.streamAdPlacer];
}

- (void)tableView:(UITableView *)tableView didEndDisplayingCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath
{
    [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with3ArgSelector:@selector(tableView:didEndDisplayingCell:forRowAtIndexPath:) firstArg:tableView secondArg:cell thirdArg:indexPath streamAdPlacer:self.streamAdPlacer];
}

- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath
{
    [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(tableView:accessoryButtonTappedForRowWithIndexPath:) firstArg:tableView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];
}

- (BOOL)tableView:(UITableView *)tableView shouldHighlightRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSInvocation *invocation = [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(tableView:shouldHighlightRowAtIndexPath:) firstArg:tableView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];

    return [MPAdPlacerInvocation boolResultForInvocation:invocation defaultValue:YES];
}

- (void)tableView:(UITableView *)tableView didHighlightRowAtIndexPath:(NSIndexPath *)indexPath
{
    [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(tableView:didHighlightRowAtIndexPath:) firstArg:tableView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];
}

- (void)tableView:(UITableView *)tableView didUnhighlightRowAtIndexPath:(NSIndexPath *)indexPath
{
    [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(tableView:didUnhighlightRowAtIndexPath:) firstArg:tableView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        return indexPath;
    }

    id<UITableViewDelegate> delegate = self.originalDelegate;
    if ([delegate respondsToSelector:@selector(tableView:willSelectRowAtIndexPath:)]) {
        NSIndexPath *origPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
        NSIndexPath *origResult = [delegate tableView:tableView willSelectRowAtIndexPath:origPath];
        return [self.streamAdPlacer adjustedIndexPathForOriginalIndexPath:origResult];
    }

    return indexPath;
}

- (NSIndexPath *)tableView:(UITableView *)tableView willDeselectRowAtIndexPath:(NSIndexPath *)indexPath
{
    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        return indexPath;
    }

    id<UITableViewDelegate> delegate = self.originalDelegate;
    if ([delegate respondsToSelector:@selector(tableView:willDeselectRowAtIndexPath:)]) {
        NSIndexPath *origPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
        NSIndexPath *origResult = [delegate tableView:tableView willDeselectRowAtIndexPath:origPath];
        return [self.streamAdPlacer adjustedIndexPathForOriginalIndexPath:origResult];
    }

    return indexPath;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        // The view inside the cell already has a gesture recognizer to handle the tap event.
        [self.tableView deselectRowAtIndexPath:indexPath animated:NO];
        return;
    }

    id<UITableViewDelegate> delegate = self.originalDelegate;
    if ([delegate respondsToSelector:@selector(tableView:didSelectRowAtIndexPath:)]) {
        NSIndexPath *originalPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
        [delegate tableView:tableView didSelectRowAtIndexPath:originalPath];
    }
}

- (void)tableView:(UITableView *)tableView didDeselectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(tableView:didDeselectRowAtIndexPath:) firstArg:tableView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath
{
    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        return UITableViewCellEditingStyleNone;
    }

    id<UITableViewDelegate> delegate = self.originalDelegate;
    if ([delegate respondsToSelector:@selector(tableView:editingStyleForRowAtIndexPath:)]) {
        NSIndexPath *origPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
        return [delegate tableView:tableView editingStyleForRowAtIndexPath:origPath];
    }

    // Apple returns UITableViewCellEditingStyleDelete by default when the cell is editable.  So we'll do the same.
    // We'll also return UITableViewCellEditingStyleNone if the cell isn't editable.
    BOOL editable = [self tableView:tableView canEditRowAtIndexPath:indexPath];

    if (editable) {
        return UITableViewCellEditingStyleDelete;
    } else {
        return UITableViewCellEditingStyleNone;
    }
}

- (NSString *)tableView:(UITableView *)tableView titleForDeleteConfirmationButtonForRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSInvocation *invocation = [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(tableView:titleForDeleteConfirmationButtonForRowAtIndexPath:) firstArg:tableView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];

    return [MPAdPlacerInvocation resultForInvocation:invocation defaultValue:@"Delete"];
}

- (BOOL)tableView:(UITableView *)tableView shouldIndentWhileEditingRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSInvocation *invocation = [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(tableView:shouldIndentWhileEditingRowAtIndexPath:) firstArg:tableView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];

    return [MPAdPlacerInvocation boolResultForInvocation:invocation defaultValue:YES];
}

- (void)tableView:(UITableView *)tableView willBeginEditingRowAtIndexPath:(NSIndexPath *)indexPath
{
    [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(tableView:willBeginEditingRowAtIndexPath:) firstArg:tableView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];
}

- (void)tableView:(UITableView *)tableView didEndEditingRowAtIndexPath:(NSIndexPath *)indexPath
{
    [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(tableView:didEndEditingRowAtIndexPath:) firstArg:tableView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];
}

- (NSInteger)tableView:(UITableView *)tableView indentationLevelForRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSInvocation *invocation = [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(tableView:indentationLevelForRowAtIndexPath:) firstArg:tableView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];

    return [MPAdPlacerInvocation integerResultForInvocation:invocation
                                               defaultValue:UITableViewCellEditingStyleNone];
}

- (BOOL)tableView:(UITableView *)tableView shouldShowMenuForRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSInvocation *invocation = [MPAdPlacerInvocation invokeForTarget:self.originalDelegate with2ArgSelector:@selector(tableView:shouldShowMenuForRowAtIndexPath:) firstArg:tableView secondArg:indexPath streamAdPlacer:self.streamAdPlacer];

    return [MPAdPlacerInvocation boolResultForInvocation:invocation defaultValue:NO];
}

- (BOOL)tableView:(UITableView *)tableView canPerformAction:(SEL)action forRowAtIndexPath:(NSIndexPath *)indexPath withSender:(id)sender
{
    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        // Can't copy or paste to an ad.
        return NO;
    }

    id<UITableViewDelegate> delegate = self.originalDelegate;
    if ([delegate respondsToSelector:@selector(tableView:canPerformAction:forRowAtIndexPath:withSender:)]) {
        NSIndexPath *origPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
        return [delegate tableView:tableView canPerformAction:action forRowAtIndexPath:origPath withSender:sender];
    }

    return NO;
}

- (void)tableView:(UITableView *)tableView performAction:(SEL)action forRowAtIndexPath:(NSIndexPath *)indexPath withSender:(id)sender {

    if ([self.streamAdPlacer isAdAtIndexPath:indexPath]) {
        // Can't copy or paste to an ad.
        return;
    }

    id<UITableViewDelegate> delegate = self.originalDelegate;
    if ([delegate respondsToSelector:@selector(tableView:performAction:forRowAtIndexPath:withSender:)]) {
        NSIndexPath *origPath = [self.streamAdPlacer originalIndexPathForAdjustedIndexPath:indexPath];
        [delegate tableView:tableView performAction:action forRowAtIndexPath:origPath withSender:sender];
    }
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    if ([self.originalDelegate respondsToSelector:@selector(tableView:viewForHeaderInSection:)]) {
        return [self.originalDelegate tableView:tableView viewForHeaderInSection:section];
    }

    return nil;
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section {
    if ([self.originalDelegate respondsToSelector:@selector(tableView:viewForFooterInSection:)]) {
        return [self.originalDelegate tableView:tableView viewForFooterInSection:section];
    }

    return nil;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
    if ([self.originalDelegate respondsToSelector:@selector(tableView:heightForHeaderInSection:)]) {
        return [self.originalDelegate tableView:tableView heightForHeaderInSection:section];
    }

    return 0;
}

- (CGFloat)tableView:(UITableView *)tableView estimatedHeightForHeaderInSection:(NSInteger)section {
    if ([self.originalDelegate respondsToSelector:@selector(tableView:estimatedHeightForHeaderInSection:)]) {
        return [self.originalDelegate tableView:tableView estimatedHeightForHeaderInSection:section];
    }

    return 0;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section {
    if ([self.originalDelegate respondsToSelector:@selector(tableView:heightForFooterInSection:)]) {
        return [self.originalDelegate tableView:tableView heightForFooterInSection:section];
    }

    return 0;
}

- (CGFloat)tableView:(UITableView *)tableView estimatedHeightForFooterInSection:(NSInteger)section {
    if ([self.originalDelegate respondsToSelector:@selector(tableView:estimatedHeightForFooterInSection:)]) {
        return [self.originalDelegate tableView:tableView estimatedHeightForFooterInSection:section];
    }

    return 0;
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

#pragma mark -

@implementation UITableView (MPTableViewAdPlacer)

static char kAdPlacerKey;

- (void)mp_setAdPlacer:(MPTableViewAdPlacer *)placer
{
    objc_setAssociatedObject(self, &kAdPlacerKey, placer, OBJC_ASSOCIATION_ASSIGN);
}

- (MPTableViewAdPlacer *)mp_adPlacer
{
    return objc_getAssociatedObject(self, &kAdPlacerKey);
}

- (void)mp_setDelegate:(id<UITableViewDelegate>)delegate
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        adPlacer.originalDelegate = delegate;
    } else {
        self.delegate = delegate;
    }
}

- (id<UITableViewDelegate>)mp_delegate
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        return adPlacer.originalDelegate;
    } else {
        return self.delegate;
    }
}

- (void)mp_setDataSource:(id<UITableViewDataSource>)dataSource
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        adPlacer.originalDataSource = dataSource;
    } else {
        self.dataSource = dataSource;
    }
}

- (id<UITableViewDataSource>)mp_dataSource
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        return adPlacer.originalDataSource;
    } else {
        return self.dataSource;
    }
}

- (void)mp_reloadData
{
    [self reloadData];
}

- (CGRect)mp_rectForRowAtIndexPath:(NSIndexPath *)indexPath
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = indexPath;

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:adjustedIndexPath];
    }

    if (!indexPath || adjustedIndexPath) {
        return [self rectForRowAtIndexPath:adjustedIndexPath];
    } else {
        return CGRectZero;
    }
}

- (NSIndexPath *)mp_indexPathForRowAtPoint:(CGPoint)point
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = [self indexPathForRowAtPoint:point];

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer originalIndexPathForAdjustedIndexPath:adjustedIndexPath];
    }

    return adjustedIndexPath;
}

- (NSIndexPath *)mp_indexPathForCell:(UITableViewCell *)cell
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = [self indexPathForCell:cell];

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer originalIndexPathForAdjustedIndexPath:adjustedIndexPath];
    }

    return adjustedIndexPath;
}

- (NSArray *)mp_indexPathsForRowsInRect:(CGRect)rect
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSArray *indexPaths = [self indexPathsForRowsInRect:rect];

    if (adPlacer) {
        indexPaths = [adPlacer.streamAdPlacer originalIndexPathsForAdjustedIndexPaths:indexPaths];
    }

    return indexPaths;
}

- (UITableViewCell *)mp_cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = indexPath;

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:adjustedIndexPath];
    }

    return [self cellForRowAtIndexPath:adjustedIndexPath];
}

- (NSArray *)mp_visibleCells
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        NSArray *indexPaths = [self mp_indexPathsForVisibleRows];
        NSMutableArray *visibleCells = [NSMutableArray array];
        for (NSIndexPath *indexPath in indexPaths) {
            UITableViewCell * cell = [self mp_cellForRowAtIndexPath:indexPath];
            if (cell != nil) {
                [visibleCells addObject:cell];
            }
        }
        return visibleCells;
    } else {
        return [self visibleCells];
    }
}

- (NSArray *)mp_indexPathsForVisibleRows
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSArray *adjustedIndexPaths = [self indexPathsForVisibleRows];

    if (adPlacer) {
        adjustedIndexPaths = [adPlacer.streamAdPlacer originalIndexPathsForAdjustedIndexPaths:adjustedIndexPaths];
    }

    return adjustedIndexPaths;
}

- (void)mp_scrollToRowAtIndexPath:(NSIndexPath *)indexPath atScrollPosition:(UITableViewScrollPosition)scrollPosition animated:(BOOL)animated
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = indexPath;

    if (adPlacer && indexPath.row != NSNotFound) {
        adjustedIndexPath = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:adjustedIndexPath];
    }

    [self scrollToRowAtIndexPath:adjustedIndexPath atScrollPosition:scrollPosition animated:animated];
}

- (void)mp_beginUpdates
{
    [self beginUpdates];
}

- (void)mp_endUpdates
{
    [self endUpdates];
}

- (void)mp_insertSections:(NSIndexSet *)sections withRowAnimation:(UITableViewRowAnimation)animation
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        [adPlacer.streamAdPlacer insertSections:sections];
    }

    [self insertSections:sections withRowAnimation:animation];
}

- (void)mp_deleteSections:(NSIndexSet *)sections withRowAnimation:(UITableViewRowAnimation)animation
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        [adPlacer.streamAdPlacer deleteSections:sections];
    }

    [self deleteSections:sections withRowAnimation:animation];
}

- (void)mp_reloadSections:(NSIndexSet *)sections withRowAnimation:(UITableViewRowAnimation)animation
{
    [self reloadSections:sections withRowAnimation:animation];
}

- (void)mp_moveSection:(NSInteger)section toSection:(NSInteger)newSection
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];

    if (adPlacer) {
        [adPlacer.streamAdPlacer moveSection:section toSection:newSection];
    }

    [self moveSection:section toSection:newSection];
}

- (void)mp_insertRowsAtIndexPaths:(NSArray *)indexPaths withRowAnimation:(UITableViewRowAnimation)animation
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSArray *adjustedIndexPaths = indexPaths;

    if (adPlacer) {
        [adPlacer.streamAdPlacer insertItemsAtIndexPaths:indexPaths];
        adjustedIndexPaths = [adPlacer.streamAdPlacer adjustedIndexPathsForOriginalIndexPaths:indexPaths];
    }

    // We perform the actual UI insertion AFTER updating the stream ad placer's
    // data, because the insertion can trigger queries to the data source, which
    // needs to reflect the post-insertion state.
    [self insertRowsAtIndexPaths:adjustedIndexPaths withRowAnimation:animation];
}

- (void)mp_deleteRowsAtIndexPaths:(NSArray *)indexPaths withRowAnimation:(UITableViewRowAnimation)animation
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSArray *adjustedIndexPaths = indexPaths;

    // We need to wrap the delete process in begin/end updates in case any ad
    // cells are also deleted. MPStreamAdPlacer's deleteItemsAtIndexPaths: can
    // call the delegate's didRemoveAdsAtIndexPaths, which will remove those
    // ads from the tableview.
    [self mp_beginUpdates];
    if (adPlacer) {
        // We need to obtain the adjusted index paths to delete BEFORE we
        // update the stream ad placer's data.
        adjustedIndexPaths = [adPlacer.streamAdPlacer adjustedIndexPathsForOriginalIndexPaths:indexPaths];
        [adPlacer.streamAdPlacer deleteItemsAtIndexPaths:indexPaths];
    }

    // We perform the actual UI deletion AFTER updating the stream ad placer's
    // data, because the deletion can trigger queries to the data source, which
    // needs to reflect the post-deletion state.
    [self deleteRowsAtIndexPaths:adjustedIndexPaths withRowAnimation:animation];
    [self mp_endUpdates];
}

- (void)mp_reloadRowsAtIndexPaths:(NSArray *)indexPaths withRowAnimation:(UITableViewRowAnimation)animation
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSArray *adjustedIndexPaths = indexPaths;

    if (adPlacer) {
        adjustedIndexPaths = [adPlacer.streamAdPlacer adjustedIndexPathsForOriginalIndexPaths:indexPaths];
    }

    [self reloadRowsAtIndexPaths:adjustedIndexPaths withRowAnimation:animation];
}

- (void)mp_moveRowAtIndexPath:(NSIndexPath *)indexPath toIndexPath:(NSIndexPath *)newIndexPath
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
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
    [self moveRowAtIndexPath:adjustedFrom toIndexPath:adjustedTo];
}

- (NSIndexPath *)mp_indexPathForSelectedRow
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = [self indexPathForSelectedRow];

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer originalIndexPathForAdjustedIndexPath:adjustedIndexPath];
    }

    return adjustedIndexPath;
}

- (NSArray *)mp_indexPathsForSelectedRows
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSArray *adjustedIndexPaths = [self indexPathsForSelectedRows];

    if (adPlacer) {
        adjustedIndexPaths = [adPlacer.streamAdPlacer originalIndexPathsForAdjustedIndexPaths:adjustedIndexPaths];
    }

    return adjustedIndexPaths;
}

- (void)mp_selectRowAtIndexPath:(NSIndexPath *)indexPath animated:(BOOL)animated scrollPosition:(UITableViewScrollPosition)scrollPosition
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = indexPath;

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:indexPath];
    }

    if (!indexPath || adjustedIndexPath) {
        [self selectRowAtIndexPath:adjustedIndexPath animated:animated scrollPosition:scrollPosition];
    }
}

- (void)mp_deselectRowAtIndexPath:(NSIndexPath *)indexPath animated:(BOOL)animated
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = indexPath;

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:indexPath];
    }

    if (!indexPath || adjustedIndexPath) {
        [self deselectRowAtIndexPath:adjustedIndexPath animated:animated];
    }
}

- (id)mp_dequeueReusableCellWithIdentifier:(NSString *)identifier forIndexPath:(NSIndexPath *)indexPath
{
    MPTableViewAdPlacer *adPlacer = [self mp_adPlacer];
    NSIndexPath *adjustedIndexPath = indexPath;

    if (adPlacer) {
        adjustedIndexPath = [adPlacer.streamAdPlacer adjustedIndexPathForOriginalIndexPath:indexPath];
    }

    if (!indexPath || adjustedIndexPath) {
        if ([self respondsToSelector:@selector(dequeueReusableCellWithIdentifier:forIndexPath:)]) {
            return [self dequeueReusableCellWithIdentifier:identifier forIndexPath:adjustedIndexPath];
        } else {
            return [self dequeueReusableCellWithIdentifier:identifier];
        }
    } else {
        return nil;
    }
}

@end
