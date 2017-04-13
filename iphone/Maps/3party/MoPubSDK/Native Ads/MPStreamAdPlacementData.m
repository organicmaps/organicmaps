//
//  MPStreamAdPlacementData.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MPStreamAdPlacementData.h"
#import "MPAdPositioning.h"
#import "MPLogging.h"

static const NSUInteger kMaximumNumberOfAdsPerStream = 255;

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MPStreamAdPlacementData ()

@property (nonatomic, strong) NSMutableDictionary *desiredOriginalPositions;
@property (nonatomic, strong) NSMutableDictionary *desiredInsertionPositions;
@property (nonatomic, strong) NSMutableDictionary *originalAdIndexPaths;
@property (nonatomic, strong) NSMutableDictionary *adjustedAdIndexPaths;
@property (nonatomic, strong) NSMutableDictionary *adDataObjects;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@implementation MPStreamAdPlacementData

- (id)initWithPositioning:(MPAdPositioning *)positioning
{
    self = [super init];
    if (self) {
        [self initializeDesiredPositionsFromPositioning:positioning];
        self.originalAdIndexPaths = [NSMutableDictionary dictionary];
        self.adjustedAdIndexPaths = [NSMutableDictionary dictionary];
        self.adDataObjects = [NSMutableDictionary dictionary];
    }
    return self;
}


- (NSMutableArray *)positioningArrayForSection:(NSUInteger)section inDictionary:(NSMutableDictionary *)dictionary
{
    NSMutableArray *array = [dictionary objectForKey:[NSNumber numberWithUnsignedInteger:section]];
    if (array) {
        return array;
    } else {
        array = [NSMutableArray array];
        [dictionary setObject:array forKey:[NSNumber numberWithUnsignedInteger:section]];
        return array;
    }
}

- (void)initializeDesiredPositionsFromPositioning:(MPAdPositioning *)positioning
{
    NSArray *fixedPositions = [[positioning.fixedPositions array] sortedArrayUsingSelector:@selector(compare:)];

    self.desiredOriginalPositions = [NSMutableDictionary dictionary];
    self.desiredInsertionPositions = [NSMutableDictionary dictionary];

    [fixedPositions enumerateObjectsUsingBlock:^(NSIndexPath *position, NSUInteger idx, BOOL *stop) {
        [self insertDesiredPositionsForIndexPath:position];
    }];

    //Current behavior only inserts repeating ads following the last fixed position in the table, and they will only be inserted
    //within the same section as that position. If no fixed positions exist, repeating ads will be placed only in the first section.
    if (positioning.repeatingInterval > 1) {
        NSInteger lastInsertionSection = [[fixedPositions lastObject] section];

        NSMutableArray *desiredOriginalPositions = [self positioningArrayForSection:lastInsertionSection inDictionary:self.desiredOriginalPositions];

        NSUInteger numberOfFixedAds = [desiredOriginalPositions count];
        NSUInteger numberOfRepeatingAds = kMaximumNumberOfAdsPerStream - numberOfFixedAds;

        NSInteger startingIndex = [fixedPositions lastObject] ? [(NSIndexPath *)[fixedPositions lastObject] row] : -1;
        for (NSUInteger repeatingAdIndex = 1; repeatingAdIndex <= numberOfRepeatingAds; repeatingAdIndex++) {
            NSInteger adIndexItem = startingIndex + positioning.repeatingInterval * repeatingAdIndex;
            [self insertDesiredPositionsForIndexPath:[NSIndexPath indexPathForRow:adIndexItem inSection:lastInsertionSection]];
        }
    }
}

//assumes items are inserted sequentially, beginning to end
- (void)insertDesiredPositionsForIndexPath:(NSIndexPath *)indexPath
{
    NSMutableArray *desiredOriginalPositions = [self positioningArrayForSection:indexPath.section inDictionary:self.desiredOriginalPositions];
    NSIndexPath *insertionIndexPath = [NSIndexPath indexPathForRow:indexPath.row - [desiredOriginalPositions count] inSection:indexPath.section];
    [desiredOriginalPositions addObject:[insertionIndexPath copy]];

    NSMutableArray *desiredInsertionPositions = [self positioningArrayForSection:indexPath.section inDictionary:self.desiredInsertionPositions];
    [desiredInsertionPositions addObject:[insertionIndexPath copy]];
}

- (NSUInteger)adjustedNumberOfItems:(NSUInteger)numberOfItems inSection:(NSUInteger)section
{
    if (numberOfItems <= 0) return 0;

    NSIndexPath *pathOfLastItem = [NSIndexPath indexPathForRow:numberOfItems - 1 inSection:section];
    NSMutableArray *originalAdIndexPaths = [self positioningArrayForSection:section inDictionary:self.originalAdIndexPaths];
    NSUInteger numberOfAdsBeforeLastItem = [self indexOfIndexPath:pathOfLastItem inSortedArray:originalAdIndexPaths options:NSBinarySearchingInsertionIndex | NSBinarySearchingLastEqual];

    return numberOfItems + numberOfAdsBeforeLastItem;
}

- (NSIndexPath *)adjustedIndexPathForOriginalIndexPath:(NSIndexPath *)indexPath
{
    if (!indexPath || indexPath.row == NSNotFound) {
        return indexPath;
    }

    NSMutableArray *originalAdIndexPaths = [self positioningArrayForSection:indexPath.section inDictionary:self.originalAdIndexPaths];
    NSUInteger numberOfAdsBeforeIndexPath = [self indexOfIndexPath:indexPath inSortedArray:originalAdIndexPaths options:NSBinarySearchingInsertionIndex | NSBinarySearchingLastEqual];

    return [NSIndexPath indexPathForRow:indexPath.row + numberOfAdsBeforeIndexPath inSection:indexPath.section];
}

- (NSIndexPath *)originalIndexPathForAdjustedIndexPath:(NSIndexPath *)indexPath
{
    if (!indexPath || indexPath.row == NSNotFound) {
        return indexPath;
    } else if ([self isAdAtAdjustedIndexPath:indexPath]) {
        return nil;
    } else {
        NSMutableArray *adjustedAdIndexPaths = [self positioningArrayForSection:indexPath.section inDictionary:self.adjustedAdIndexPaths];
        NSUInteger numberOfAdsBeforeIndexPath = [self indexOfIndexPath:indexPath inSortedArray:adjustedAdIndexPaths options:NSBinarySearchingInsertionIndex];
        return [NSIndexPath indexPathForRow:indexPath.row - numberOfAdsBeforeIndexPath inSection:indexPath.section];
    }
}

- (NSIndexPath *)nextAdInsertionIndexPathForAdjustedIndexPath:(NSIndexPath *)adjustedIndexPath
{
    if (adjustedIndexPath.section > [self largestSectionIndexContainingAds]) {
        return nil;
    }

    NSMutableArray *desiredInsertionPositions = [self.desiredInsertionPositions objectForKey:[NSNumber numberWithUnsignedInteger:adjustedIndexPath.section]];

    NSUInteger index = [self indexOfIndexPath:adjustedIndexPath inSortedArray:desiredInsertionPositions options:NSBinarySearchingInsertionIndex | NSBinarySearchingFirstEqual];

    if (desiredInsertionPositions && (index < [desiredInsertionPositions count])) {
        return [desiredInsertionPositions objectAtIndex:index];
    } else {
        // Go to the next section.
        return [self nextAdInsertionIndexPathForAdjustedIndexPath:[NSIndexPath indexPathForRow:0 inSection:adjustedIndexPath.section+1]];
    }
}

- (NSIndexPath *)previousAdInsertionIndexPathForAdjustedIndexPath:(NSIndexPath *)adjustedIndexPath
{
    NSMutableArray *desiredInsertionPositions = [self positioningArrayForSection:adjustedIndexPath.section inDictionary:self.desiredInsertionPositions];
    NSUInteger index = [self indexOfIndexPath:adjustedIndexPath inSortedArray:desiredInsertionPositions options:NSBinarySearchingInsertionIndex | NSBinarySearchingFirstEqual];

    if (index > 0) {
        return desiredInsertionPositions[index - 1];
    } else {
        return nil;
    }
}

- (void)insertAdData:(MPNativeAdData *)data atIndexPath:(NSIndexPath *)adjustedIndexPath
{
    NSMutableArray *desiredInsertionPositions = [self positioningArrayForSection:adjustedIndexPath.section inDictionary:self.desiredInsertionPositions];
    NSMutableArray *desiredOriginalPositions = [self positioningArrayForSection:adjustedIndexPath.section inDictionary:self.desiredOriginalPositions];
    NSMutableArray *adjustedAdIndexPaths = [self positioningArrayForSection:adjustedIndexPath.section inDictionary:self.adjustedAdIndexPaths];
    NSMutableArray *originalAdIndexPaths = [self positioningArrayForSection:adjustedIndexPath.section inDictionary:self.originalAdIndexPaths];
    NSMutableArray *adDataObjects = [self positioningArrayForSection:adjustedIndexPath.section inDictionary:self.adDataObjects];

    NSUInteger indexInDesiredArrays = [self indexOfIndexPath:adjustedIndexPath inSortedArray:desiredInsertionPositions options:NSBinarySearchingFirstEqual];

    if (indexInDesiredArrays == NSNotFound) {
        MPLogWarn(@"Attempted to insert an ad at position %@, which is not in the desired array.", adjustedIndexPath);
        return;
    }

    NSIndexPath *originalPosition = desiredOriginalPositions[indexInDesiredArrays];
    NSIndexPath *insertionPosition = desiredInsertionPositions[indexInDesiredArrays];

    NSUInteger insertionIndex = [self indexOfIndexPath:insertionPosition inSortedArray:adjustedAdIndexPaths options:NSBinarySearchingInsertionIndex | NSBinarySearchingFirstEqual];

    [originalAdIndexPaths insertObject:originalPosition atIndex:insertionIndex];
    [adjustedAdIndexPaths insertObject:insertionPosition atIndex:insertionIndex];
    [adDataObjects insertObject:data atIndex:insertionIndex];

    [desiredOriginalPositions removeObjectAtIndex:indexInDesiredArrays];
    [desiredInsertionPositions removeObjectAtIndex:indexInDesiredArrays];

    for (NSUInteger i = insertionIndex + 1; i < [adjustedAdIndexPaths count]; i++) {
        NSIndexPath *newIndexPath = adjustedAdIndexPaths[i];
        adjustedAdIndexPaths[i] = [NSIndexPath indexPathForRow:newIndexPath.row + 1 inSection:newIndexPath.section];
    }

    for (NSUInteger j = indexInDesiredArrays; j < [desiredInsertionPositions count]; j++) {
        NSIndexPath *newInsertionPosition = desiredInsertionPositions[j];
        desiredInsertionPositions[j] = [NSIndexPath indexPathForRow:newInsertionPosition.row + 1 inSection:newInsertionPosition.section];
    }
}

- (NSArray *)adjustedAdIndexPathsInAdjustedRange:(NSRange)range inSection:(NSInteger)section
{
    NSMutableArray *adjustedIndexPaths = [self positioningArrayForSection:section inDictionary:self.adjustedAdIndexPaths];

    NSIndexSet *indexesOfObjectsInRange = [adjustedIndexPaths indexesOfObjectsPassingTest:^BOOL(NSIndexPath *adjustedIndexPath, NSUInteger idx, BOOL *stop) {
        return NSLocationInRange(adjustedIndexPath.row, range);
    }];

    return [adjustedIndexPaths objectsAtIndexes:indexesOfObjectsInRange];
}

- (void)clearAdsInAdjustedRange:(NSRange)range inSection:(NSInteger)section
{
    NSMutableArray *adjustedAdIndexPaths = [self positioningArrayForSection:section inDictionary:self.adjustedAdIndexPaths];
    NSMutableArray *originalAdIndexPaths = [self positioningArrayForSection:section inDictionary:self.originalAdIndexPaths];
    NSMutableArray *adDataObjects = [self positioningArrayForSection:section inDictionary:self.adDataObjects];
    NSMutableArray *desiredInsertionPositions = [self positioningArrayForSection:section inDictionary:self.desiredInsertionPositions];
    NSMutableArray *desiredOriginalPositions = [self positioningArrayForSection:section inDictionary:self.desiredOriginalPositions];

    NSIndexSet *indexesOfObjectsToRemove = [adjustedAdIndexPaths indexesOfObjectsPassingTest:^BOOL(NSIndexPath *adjustedIndexPath, NSUInteger idx, BOOL *stop) {
        return NSLocationInRange(adjustedIndexPath.row, range);
    }];

    if ([indexesOfObjectsToRemove count]) {
        [indexesOfObjectsToRemove enumerateIndexesWithOptions:NSEnumerationReverse usingBlock:^(NSUInteger idx, BOOL *stop) {
            NSIndexPath *adjustedIndexPathToRemove = adjustedAdIndexPaths[idx];
            NSIndexPath *originalIndexPathToRemove = originalAdIndexPaths[idx];
            NSUInteger insertionIndex = [self indexOfIndexPath:originalIndexPathToRemove inSortedArray:desiredOriginalPositions options:NSBinarySearchingInsertionIndex | NSBinarySearchingFirstEqual];
            for (NSInteger i = insertionIndex; i < [desiredInsertionPositions count]; i++) {
                NSIndexPath *nextIndexPath = desiredInsertionPositions[i];
                desiredInsertionPositions[i] = [NSIndexPath indexPathForRow:nextIndexPath.row - 1 inSection:nextIndexPath.section];
            }
            [desiredOriginalPositions insertObject:originalIndexPathToRemove atIndex:insertionIndex];
            [desiredInsertionPositions insertObject:adjustedIndexPathToRemove atIndex:insertionIndex];

        }];

        [adjustedAdIndexPaths removeObjectsAtIndexes:indexesOfObjectsToRemove];
        [originalAdIndexPaths removeObjectsAtIndexes:indexesOfObjectsToRemove];
        [adDataObjects removeObjectsAtIndexes:indexesOfObjectsToRemove];
    }
}

- (BOOL)isAdAtAdjustedIndexPath:(NSIndexPath *)adjustedIndexPath
{
    NSMutableArray *adjustedAdIndexPaths = [self positioningArrayForSection:adjustedIndexPath.section inDictionary:self.adjustedAdIndexPaths];
    NSUInteger indexOfIndexPath = [self indexOfIndexPath:adjustedIndexPath inSortedArray:adjustedAdIndexPaths options:NSBinarySearchingFirstEqual];

    return indexOfIndexPath != NSNotFound;
}

- (NSArray *)adjustedIndexPathsWithAdsInSection:(NSUInteger)section
{
    return [self positioningArrayForSection:section inDictionary:self.adjustedAdIndexPaths];
}

- (MPNativeAdData *)adDataAtAdjustedIndexPath:(NSIndexPath *)adjustedIndexPath
{
    NSMutableArray *adjustedAdIndexPaths = [self positioningArrayForSection:adjustedIndexPath.section inDictionary:self.adjustedAdIndexPaths];
    NSMutableArray *adDataObjects = [self positioningArrayForSection:adjustedIndexPath.section inDictionary:self.adDataObjects];

    NSUInteger indexOfIndexPath = [self indexOfIndexPath:adjustedIndexPath inSortedArray:adjustedAdIndexPaths options:NSBinarySearchingFirstEqual];

    if (indexOfIndexPath != NSNotFound) {
        return adDataObjects[indexOfIndexPath];
    } else {
        return nil;
    }
}

- (void)insertSections:(NSIndexSet *)sections
{
    [sections enumerateIndexesUsingBlock:^(NSUInteger insertionSection, BOOL *stop) {
        // Explicitly casting indices to NSInteger because we're counting backwards.
        NSInteger maxSection = [self largestSectionIndexContainingAds];
        NSInteger signedInsertionSection = insertionSection;

        // We need to shift all the data above the new section up by 1.
        for (NSInteger i = maxSection; i >= signedInsertionSection; --i) {
            if (self.desiredInsertionPositions[@(i)]) {
                [self moveAllPositioningArraysInDictionariesAtSection:i toSection:i+1];
            }
        }
    }];
}

- (void)deleteSections:(NSIndexSet *)sections
{
    [sections enumerateIndexesWithOptions:NSEnumerationReverse usingBlock:^(NSUInteger deletionSection, BOOL *stop) {
        NSUInteger maxSection = [self largestSectionIndexContainingAds];

        [self clearPositioningArraysInDictionariesAtSection:deletionSection];

        // We need to shift all the data above the deletionSection down by 1.
        for (NSUInteger i = deletionSection; i < maxSection; ++i) {
            if (self.desiredInsertionPositions[@(i+1)]) {
                [self moveAllPositioningArraysInDictionariesAtSection:i+1 toSection:i];
            }
        }
    }];
}

- (void)moveSection:(NSInteger)section toSection:(NSInteger)newSection
{
    // Store the data at the section we're moving and retain it so it doesn't get deleted.
    NSMutableArray *desiredInsertionPositions = [self positioningArrayForSection:section inDictionary:self.desiredInsertionPositions];
    NSMutableArray *desiredOriginalPositions = [self positioningArrayForSection:section inDictionary:self.desiredOriginalPositions];
    NSMutableArray *adjustedAdIndexPaths = [self positioningArrayForSection:section inDictionary:self.adjustedAdIndexPaths];
    NSMutableArray *originalAdIndexPaths = [self positioningArrayForSection:section inDictionary:self.originalAdIndexPaths];
    NSMutableArray *adDataObjects = [self positioningArrayForSection:section inDictionary:self.adDataObjects];

    // Delete it from our dictionaries.
    [self deleteSections:[NSIndexSet indexSetWithIndex:section]];

    // Now insert an empty section at the new spot.
    [self insertSections:[NSIndexSet indexSetWithIndex:newSection]];

    // Fill in its data.
    self.desiredInsertionPositions[@(newSection)] = desiredInsertionPositions;
    self.desiredOriginalPositions[@(newSection)] = desiredOriginalPositions;
    self.adjustedAdIndexPaths[@(newSection)] = adjustedAdIndexPaths;
    self.originalAdIndexPaths[@(newSection)] = originalAdIndexPaths;
    self.adDataObjects[@(newSection)] = adDataObjects;

    [self updateAllSectionsForPositioningArraysAtSection:newSection];
}

- (void)insertItemsAtIndexPaths:(NSArray *)originalIndexPaths
{
    originalIndexPaths = [originalIndexPaths sortedArrayUsingSelector:@selector(compare:)];

    [originalIndexPaths enumerateObjectsUsingBlock:^(NSIndexPath *originalIndexPath, NSUInteger idx, BOOL *stop) {
        NSMutableArray *desiredOriginalPositions = [self positioningArrayForSection:originalIndexPath.section inDictionary:self.desiredOriginalPositions];
        NSMutableArray *originalAdIndexPaths = [self positioningArrayForSection:originalIndexPath.section inDictionary:self.originalAdIndexPaths];

        NSUInteger insertionIndex = [self indexOfIndexPath:originalIndexPath inSortedArray:desiredOriginalPositions options:NSBinarySearchingInsertionIndex | NSBinarySearchingFirstEqual];
        for (NSUInteger i = insertionIndex; i < [desiredOriginalPositions count]; i++) {
            [self incrementDesiredIndexPathsAtIndex:i inSection:originalIndexPath.section];
        }

        NSUInteger originalInsertionIndex = [self indexOfIndexPath:originalIndexPath inSortedArray:originalAdIndexPaths options:NSBinarySearchingInsertionIndex | NSBinarySearchingFirstEqual];
        for (NSUInteger i = originalInsertionIndex; i < [originalAdIndexPaths count]; i++) {
            [self incrementPlacedIndexPathsAtIndex:i inSection:originalIndexPath.section];
        }
    }];
}

- (void)deleteItemsAtIndexPaths:(NSArray *)originalIndexPaths
{
    originalIndexPaths = [originalIndexPaths sortedArrayUsingSelector:@selector(compare:)];

    __block NSUInteger currentNumberOfAdsInSection = 0;
    __block NSInteger lastSection = [[originalIndexPaths firstObject] section];

    [originalIndexPaths enumerateObjectsUsingBlock:^(NSIndexPath *originalIndexPath, NSUInteger idx, BOOL *stop) {
        // Batch deletions are actually performed one at a time. This requires us to shift up the
        // deletion index paths each time a deletion is performed.
        //
        // For example, batch-deleting the 2nd and 3rd items is not equivalent to incrementally
        // deleting the 2nd item and then the 3rd item; the equivalent would be to delete the 2nd
        // item each time.
        if (originalIndexPath.section != lastSection) {
            lastSection = originalIndexPath.section;
            currentNumberOfAdsInSection = 0;
        }

        NSMutableArray *desiredOriginalPositions = [self positioningArrayForSection:originalIndexPath.section inDictionary:self.desiredOriginalPositions];
        NSMutableArray *originalAdIndexPaths = [self positioningArrayForSection:originalIndexPath.section inDictionary:self.originalAdIndexPaths];

        NSIndexPath *indexPathOfItemToDelete = [NSIndexPath indexPathForRow:originalIndexPath.row - currentNumberOfAdsInSection inSection:originalIndexPath.section];

        NSUInteger searchIndexInDesired = [self indexOfIndexPath:indexPathOfItemToDelete inSortedArray:desiredOriginalPositions options:NSBinarySearchingInsertionIndex | NSBinarySearchingLastEqual];
        for (NSUInteger i = searchIndexInDesired; i < [desiredOriginalPositions count]; i++) {
            [self decrementDesiredIndexPathsAtIndex:i inSection:originalIndexPath.section];
        }

        NSUInteger searchIndexInPlaced = [self indexOfIndexPath:indexPathOfItemToDelete inSortedArray:originalAdIndexPaths options:NSBinarySearchingInsertionIndex | NSBinarySearchingLastEqual];
        for (NSUInteger i = searchIndexInPlaced; i < [originalAdIndexPaths count]; i++) {
            [self decrementPlacedIndexPathsAtIndex:i inSection:originalIndexPath.section];
        }

        currentNumberOfAdsInSection++;
    }];
}

- (void)moveItemAtIndexPath:(NSIndexPath *)originalIndexPath toIndexPath:(NSIndexPath *)newIndexPath
{
    [self deleteItemsAtIndexPaths:@[originalIndexPath]];
    [self insertItemsAtIndexPaths:@[newIndexPath]];
}

#pragma mark - NSIndexPath array helpers

- (NSUInteger)indexOfIndexPath:(NSIndexPath *)indexPath inSortedArray:(NSArray *)array options:(NSBinarySearchingOptions)options
{
    if (!indexPath || indexPath.row == NSNotFound) {
        return NSNotFound;
    }
    return [array indexOfObject:indexPath inSortedRange:NSMakeRange(0, [array count]) options:options usingComparator:^NSComparisonResult(NSIndexPath *path1, NSIndexPath *path2) {
        return [path1 compare:path2];
    }];
}

- (void)incrementDesiredIndexPathsAtIndex:(NSUInteger)index inSection:(NSUInteger)section
{
    NSMutableArray *desiredOriginalPositions = [self positioningArrayForSection:section inDictionary:self.desiredOriginalPositions];
    NSMutableArray *desiredInsertionPositions = [self positioningArrayForSection:section inDictionary:self.desiredInsertionPositions];

    NSIndexPath *currentDesiredOriginalPosition = desiredOriginalPositions[index];
    NSIndexPath *newDesiredOriginalPosition = [NSIndexPath indexPathForRow:currentDesiredOriginalPosition.row + 1 inSection:currentDesiredOriginalPosition.section];
    desiredOriginalPositions[index] = newDesiredOriginalPosition;

    NSIndexPath *currentDesiredInsertionPosition = desiredInsertionPositions[index];
    NSIndexPath *newDesiredInsertionPosition = [NSIndexPath indexPathForRow:currentDesiredInsertionPosition.row + 1 inSection:currentDesiredInsertionPosition.section];
    desiredInsertionPositions[index] = newDesiredInsertionPosition;
}

- (void)incrementPlacedIndexPathsAtIndex:(NSUInteger)index inSection:(NSUInteger)section
{
    NSMutableArray *originalAdIndexPaths = [self positioningArrayForSection:section inDictionary:self.originalAdIndexPaths];
    NSMutableArray *adjustedAdIndexPaths = [self positioningArrayForSection:section inDictionary:self.adjustedAdIndexPaths];

    NSIndexPath *currentOriginalIndexPath = originalAdIndexPaths[index];
    NSIndexPath *newOriginalIndexPath = [NSIndexPath indexPathForRow:currentOriginalIndexPath.row + 1 inSection:currentOriginalIndexPath.section];
    originalAdIndexPaths[index] = newOriginalIndexPath;

    NSIndexPath *currentAdjustedIndexPath = adjustedAdIndexPaths[index];
    NSIndexPath *newAdjustedIndexPath = [NSIndexPath indexPathForRow:currentAdjustedIndexPath.row + 1 inSection:currentAdjustedIndexPath.section];
    adjustedAdIndexPaths[index] = newAdjustedIndexPath;
}

- (void)decrementDesiredIndexPathsAtIndex:(NSUInteger)index inSection:(NSUInteger)section
{
    NSMutableArray *desiredOriginalPositions = [self positioningArrayForSection:section inDictionary:self.desiredOriginalPositions];
    NSMutableArray *desiredInsertionPositions = [self positioningArrayForSection:section inDictionary:self.desiredInsertionPositions];

    NSIndexPath *currentDesiredOriginalPosition = desiredOriginalPositions[index];
    NSIndexPath *newDesiredOriginalPosition = [NSIndexPath indexPathForRow:currentDesiredOriginalPosition.row - 1 inSection:currentDesiredOriginalPosition.section];
    desiredOriginalPositions[index] = newDesiredOriginalPosition;

    NSIndexPath *currentDesiredInsertionPosition = desiredInsertionPositions[index];
    NSIndexPath *newDesiredInsertionPosition = [NSIndexPath indexPathForRow:currentDesiredInsertionPosition.row - 1 inSection:currentDesiredInsertionPosition.section];
    desiredInsertionPositions[index] = newDesiredInsertionPosition;
}

- (void)decrementPlacedIndexPathsAtIndex:(NSUInteger)index inSection:(NSUInteger)section
{
    NSMutableArray *originalAdIndexPaths = [self positioningArrayForSection:section inDictionary:self.originalAdIndexPaths];
    NSMutableArray *adjustedAdIndexPaths = [self positioningArrayForSection:section inDictionary:self.adjustedAdIndexPaths];

    NSIndexPath *currentOriginalIndexPath = originalAdIndexPaths[index];
    NSIndexPath *newOriginalIndexPath = [NSIndexPath indexPathForRow:currentOriginalIndexPath.row - 1 inSection:currentOriginalIndexPath.section];
    originalAdIndexPaths[index] = newOriginalIndexPath;

    NSIndexPath *currentAdjustedIndexPath = adjustedAdIndexPaths[index];
    NSIndexPath *newAdjustedIndexPath = [NSIndexPath indexPathForRow:currentAdjustedIndexPath.row - 1 inSection:currentAdjustedIndexPath.section];
    adjustedAdIndexPaths[index] = newAdjustedIndexPath;
}

#pragma mark - Section modification helpers

- (NSUInteger)largestSectionIndexContainingAds
{
    return [[[self.desiredInsertionPositions allKeys] valueForKeyPath:@"@max.unsignedIntValue"] unsignedIntegerValue];
}

// Does not update the index path sections.  Call updateSectionForPositioningArray to update the sections in the index paths.
- (void)copyPositioningArrayInDictionary:(NSMutableDictionary *)dict atSection:(NSUInteger)atSection toSection:(NSUInteger)toSection
{
    if (dict[@(atSection)]) {
        dict[@(toSection)] = dict[@(atSection)];
    }
}

- (void)clearPositioningArraysInDictionariesAtSection:(NSUInteger)section
{
    [self.desiredInsertionPositions removeObjectForKey:@(section)];
    [self.desiredOriginalPositions removeObjectForKey:@(section)];
    [self.adjustedAdIndexPaths removeObjectForKey:@(section)];
    [self.originalAdIndexPaths removeObjectForKey:@(section)];
    [self.adDataObjects removeObjectForKey:@(section)];
}

// Moves the positioning arrays to the correct spots in the dictionaries and also updates the sections for all the index paths.
- (void)moveAllPositioningArraysInDictionariesAtSection:(NSUInteger)atSection toSection:(NSUInteger)toSection
{
    [self copyPositioningArrayInDictionary:self.desiredInsertionPositions atSection:atSection toSection:toSection];
    [self copyPositioningArrayInDictionary:self.desiredOriginalPositions atSection:atSection toSection:toSection];
    [self copyPositioningArrayInDictionary:self.adjustedAdIndexPaths atSection:atSection toSection:toSection];
    [self copyPositioningArrayInDictionary:self.originalAdIndexPaths atSection:atSection toSection:toSection];
    [self copyPositioningArrayInDictionary:self.adDataObjects atSection:atSection toSection:toSection];

    [self updateAllSectionsForPositioningArraysAtSection:toSection];

    [self clearPositioningArraysInDictionariesAtSection:atSection];
}

- (void)updateAllSectionsForPositioningArraysAtSection:(NSUInteger)section
{
    [self updateSectionForPositioningArray:self.desiredInsertionPositions[@(section)] toSection:section];
    [self updateSectionForPositioningArray:self.desiredOriginalPositions[@(section)] toSection:section];
    [self updateSectionForPositioningArray:self.adjustedAdIndexPaths[@(section)] toSection:section];
    [self updateSectionForPositioningArray:self.originalAdIndexPaths[@(section)] toSection:section];
}

- (void)updateSectionForPositioningArray:(NSMutableArray *)positioningArray toSection:(NSUInteger)section
{
    for (NSUInteger i = 0; i < positioningArray.count; ++i) {
        NSIndexPath *indexPath = positioningArray[i];

        NSUInteger indices[] = { section, [indexPath indexAtPosition:1] };
        positioningArray[i] = [NSIndexPath indexPathWithIndexes:indices length:2];
    }
}

@end
