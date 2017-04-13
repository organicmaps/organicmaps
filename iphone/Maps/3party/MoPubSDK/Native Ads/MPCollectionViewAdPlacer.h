//
//  MPCollectionViewAdPlacer.h
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPClientAdPositioning.h"
#import "MPServerAdPositioning.h"

@class MPNativeAdRequestTargeting;
@protocol MPCollectionViewAdPlacerDelegate;

////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * The `MPCollectionViewAdPlacer` class allows you to request native ads from the MoPub ad server
 * and place them into a `UICollectionView` object.
 *
 * When an instance of this class is initialized with a collection view, it wraps the collection
 * view's data source and delegate in order to insert ads and adjust the positions of your regular
 * content cells.
 */

@interface MPCollectionViewAdPlacer : NSObject

@property (nonatomic, weak) id<MPCollectionViewAdPlacerDelegate> delegate;

/** @name Initializing a Collection View Ad Placer */

/**
 * Creates and returns an ad placer that will insert ads into a collection view at positions that can
 * be configured dynamically on the MoPub website.
 *
 * When you make an ad request, the ad placer will ask the MoPub ad server for the positions where
 * ads should be inserted into the collection view. You can configure these positioning values by
 * editing your ad unit's settings on the MoPub website.
 *
 * Using this method is equivalent to calling
 * +placerWithCollectionView:viewController:adPositioning:defaultAdRenderingClass: and passing in an
 * `MPServerAdPositioning` object as the `positioning` parameter.
 *
 * @param collectionView The collection view in which to insert ads.
 * @param controller The view controller which should be used to present modal content.
 * @param rendererConfigurations An array of MPNativeAdRendererConfiguration objects that control how
 * the native ad is rendered.
 *
 * @return An `MPCollectionViewAdPlacer` object.
 */
+ (instancetype)placerWithCollectionView:(UICollectionView *)collectionView viewController:(UIViewController *)controller rendererConfigurations:(NSArray *)rendererConfigurations;

/**
 * Creates and returns an ad placer that will insert ads into a collection view.
 *
 * When using this method, there are two options for controlling the positions where ads appear
 * within the collection view.
 *
 * First, you may pass an `MPServerAdPositioning` object as the `positioning` parameter, which tells
 * the ad placer to obtain positioning information dynamically from the ad server, which you can
 * configure on the MoPub website. In many cases, this is the preferred approach, since it allows
 * you to modify the positions without rebuilding your application. Note that calling the
 * convenience method +placerWithCollectionView:viewController:defaultAdRenderingClass: accomplishes
 * this as well.
 *
 * Alternatively, if you wish to hard-code your positions, you may pass an `MPClientAdPositioning`
 * object instead.
 *
 * @param collectionView The collection view in which to insert ads.
 * @param controller The view controller which should be used to present modal content.
 * @param positioning The positioning object that specifies where ads should be shown in the stream.
 * @param rendererConfigurations An array of MPNativeAdRendererConfiguration objects that control how
 * the native ad is rendered.
 *
 * @return An `MPCollectionViewAdPlacer` object.
 */
+ (instancetype)placerWithCollectionView:(UICollectionView *)collectionView viewController:(UIViewController *)controller adPositioning:(MPAdPositioning *)positioning rendererConfigurations:(NSArray *)rendererConfigurations;

/** @name Requesting Ads */

/**
 * Requests ads from the MoPub ad server using the specified ad unit ID.
 *
 * @param adUnitID A string representing a MoPub ad unit ID.
 */
- (void)loadAdsForAdUnitID:(NSString *)adUnitID;

/**
 * Requests ads from the MoPub ad server using the specified ad unit ID and targeting parameters.
 *
 * @param adUnitID A string representing a MoPub ad unit ID.
 * @param targeting An object containing targeting information, such as geolocation data.
 */
- (void)loadAdsForAdUnitID:(NSString *)adUnitID targeting:(MPNativeAdRequestTargeting *)targeting;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * The MoPub SDK adds interfaces to the `UICollectionView` class to help your application with
 * responsibilities related to `MPCollectionViewAdPlacer`. These APIs include methods to help notify
 * the ad placer of all modifications to the original collection view, as well as to simplify your
 * application code such that it does not need to perform index path manipulations to account for
 * the presence of ads.
 *
 * Since the ad placer replaces the original data source and delegate objects of your collection
 * view, the SDK also provides new methods for you to set these properties such that the ad placer
 * remains aware of the changes.
 */

@interface UICollectionView (MPCollectionViewAdPlacer)

- (void)mp_setAdPlacer:(MPCollectionViewAdPlacer *)placer;

/** @name Obtaining the Collection View Ad Placer */

/**
 * Returns the ad placer currently being used for this collection view.
 *
 * @return An ad placer object or `nil` if no ad placer is being used.
 */
- (MPCollectionViewAdPlacer *)mp_adPlacer;

/** @name Setting and Getting the Delegate and Data Source */

/**
 * Sets the collection view's data source.
 *
 * If your application needs to change a collection view's data source after it has instantiated an
 * ad placer using that collection view, use this method rather than
 * -[UICollectionView setDataSource:].
 *
 * @param dataSource The new collection view data source.
 */
- (void)mp_setDataSource:(id<UICollectionViewDataSource>)dataSource;

/**
 * Returns the original data source of the collection view.
 *
 * When you instantiate an ad placer using a collection view, the ad placer replaces the collection
 * view's original data source object. If your application needs to access the original data source,
 * use this method instead of -[UICollectionView dataSource].
 *
 * @return The original collection view data source.
 */
- (id<UICollectionViewDataSource>)mp_dataSource;

/**
 * Sets the collection view's delegate.
 *
 * If your application needs to change a collection view's delegate after it has instantiated an ad
 * placer using that collection view, use this method rather than -[UICollectionView setDelegate:].
 *
 * @param delegate The new collection view delegate.
 */
- (void)mp_setDelegate:(id<UICollectionViewDelegate>)delegate;

/**
 * Returns the original delegate of the collection view.
 *
 * When you instantiate an ad placer using a collection view, the ad placer replaces the collection
 * view's original delegate object. If your application needs to access the original delegate, use
 * this method instead of -[UICollectionView delegate].
 *
 * @return The original collection view delegate.
 */
- (id<UICollectionViewDelegate>)mp_delegate;

/** @name Notifying the Collection View Ad Placer of Content Changes */

/**
 * Reloads all of the data for the collection view.
 */
- (void)mp_reloadData;

/**
 * Inserts new items at the specified index paths, and informs the attached ad placer of the
 * insertions.
 *
 * @param indexPaths An array of `NSIndexPath` objects, each of which contains a section index and
 * item index at which to insert a new cell. This parameter must not be `nil`.
 */
- (void)mp_insertItemsAtIndexPaths:(NSArray *)indexPaths;

/**
 * Deletes the items at the specified index paths, and informs the attached ad placer of the
 * deletions.
 *
 * @param indexPaths An array of `NSIndexPath` objects, each of which contains a section index and
 * item index for the item you want to delete from the collection view. This parameter must not be
 * `nil`.
 */
- (void)mp_deleteItemsAtIndexPaths:(NSArray *)indexPaths;

/**
 * Reloads just the items at the specified index paths.
 *
 * @param indexPaths An array of `NSIndexPath` objects identifying the items you want to update.
 */
- (void)mp_reloadItemsAtIndexPaths:(NSArray *)indexPaths;

/**
 * Moves an item from one location to another in the collection view, taking into account ads
 * inserted by the ad placer.
 *
 * @param indexPath    The index path of the item you want to move. This parameter must not be
 * `nil`.
 * @param newIndexPath The index path of the item’s new location. This parameter must not be `nil`.
 */
- (void)mp_moveItemAtIndexPath:(NSIndexPath *)indexPath toIndexPath:(NSIndexPath *)newIndexPath;

/**
 * Inserts new sections at the specified indexes, and informs the attached ad placer of the
 * insertions.
 *
 * @param sections An index set containing the indexes of the sections you want to insert. This
 * parameter must not be `nil`.
 */
- (void)mp_insertSections:(NSIndexSet *)sections;

/**
 * Deletes the sections at the specified indexes, and informs the attached ad placer of the
 * deletions.
 *
 * @param sections The indexes of the sections you want to delete. This parameter must not be `nil`.
 */
- (void)mp_deleteSections:(NSIndexSet *)sections;

/**
 * Reloads the data in the specified sections of the collection view, and informs the attached ad
 * placer that sections may have changed.
 *
 * @param sections The indexes of the sections to reload.
 */
- (void)mp_reloadSections:(NSIndexSet *)sections;

/**
 * Moves a section from one location to another in the collection view, and informs the attached ad
 * placer.
 *
 * @param section    The index path of the section you want to move. This parameter must not be
 * `nil`.
 * @param newSection The index path of the section’s new location. This parameter must not be `nil`.
 */
- (void)mp_moveSection:(NSInteger)section toSection:(NSInteger)newSection;

/** @name Methods Involving Index Paths */

/**
 * Returns the visible cell object at the specified index path.
 *
 * @param indexPath The index path that specifies the section and item number of the cell.
 *
 * @return The cell object at the corresponding index path or `nil` if the cell is not visible or
 * *indexPath* is out of range.
 */
- (UICollectionViewCell *)mp_cellForItemAtIndexPath:(NSIndexPath *)indexPath;

/**
 * Returns a reusable cell object located by its identifier.
 *
 * @param identifier The reuse identifier for the specified cell. This parameter must not be `nil`.
 * @param indexPath  The index path specifying the location of the cell. The data source receives
 * this information when it is asked for the cell and should just pass it along. This method uses
 * the index path to perform additional configuration based on the cell’s position in the collection
 * view.
 *
 * @return A valid `UICollectionReusableView` object.
 */
- (id)mp_dequeueReusableCellWithReuseIdentifier:(NSString *)identifier forIndexPath:(NSIndexPath*)indexPath;

/**
 * Deselects the item at the specified index.
 *
 * @param indexPath The index path of the item to select. Specifying `nil` results in no change to
 * the current selection.
 * @param animated  Specify YES to animate the change in the selection or NO to make the change
 * without animating it.
 */
- (void)mp_deselectItemAtIndexPath:(NSIndexPath *)indexPath animated:(BOOL)animated;

/**
 * Returns the original index path for the given cell or `nil` if the cell is an ad cell.
 *
 * @param cell The cell object whose index path you want.
 *
 * @return The index path of the cell or `nil` if the specified cell contains an ad or is not in the
 * collection view.
 */
- (NSIndexPath *)mp_indexPathForCell:(UICollectionViewCell *)cell;

/**
 * Returns the index path of the item at the specified point in the collection view.
 *
 * @param point A point in the collection view’s coordinate system.
 *
 * @return The index path of the item at the specified point or `nil` if either an ad or no item was
 * found at the specified point.
 */
- (NSIndexPath *)mp_indexPathForItemAtPoint:(CGPoint)point;

/**
 * Returns the original index paths (as if no ads were inserted) for the selected items.
 *
 * @return An array of the original index paths for the selected items.
 */
- (NSArray *)mp_indexPathsForSelectedItems;

/**
 * Returns an array of original index paths each identifying a visible non-ad item in the collection
 * view, calculated before any ads were inserted.
 *
 * @return An array of the original index paths representing visible non-ad items in the collection
 * view. Returns `nil` if no items are visible.
 */
- (NSArray *)mp_indexPathsForVisibleItems;

/**
 * Returns the layout information for the item at the specified index path.
 *
 * @param indexPath The index path of the item.
 *
 * @return The layout attributes for the item or `nil` if no item exists at the specified path.
 */
- (UICollectionViewLayoutAttributes *)mp_layoutAttributesForItemAtIndexPath:(NSIndexPath *)indexPath;

/**
 * Scrolls the collection view contents until the specified item is visible.
 *
 * @param indexPath      The index path of the item to scroll into view.
 * @param scrollPosition An option that specifies where the item should be positioned when scrolling
 * finishes.
 * @param animated       Specify YES to animate the scrolling behavior or NO to adjust the scroll
 * view’s visible content immediately.
 */
- (void)mp_scrollToItemAtIndexPath:(NSIndexPath *)indexPath atScrollPosition:(UICollectionViewScrollPosition)scrollPosition animated:(BOOL)animated;

/**
 * Selects the item at the specified index path and optionally scrolls it into view.
 *
 * @param indexPath      The index path of the item to select. Specifying `nil` for this parameter
 * clears the current selection.
 * @param animated       Specify YES to animate the change in the selection or NO to make the change
 * without animating it.
 * @param scrollPosition An option that specifies where the item should be positioned when scrolling
 * finishes.
 */
- (void)mp_selectItemAtIndexPath:(NSIndexPath *)indexPath animated:(BOOL)animated scrollPosition:(UICollectionViewScrollPosition)scrollPosition;

/**
 * Returns an array of the non-ad cells that are visible in the collection view.
 *
 * @return An array of `UICollectionViewCell` objects, each representing a visible, non-ad cell in
 * the receiving collection view.
 */
- (NSArray *)mp_visibleCells;

@end

@protocol MPCollectionViewAdPlacerDelegate <NSObject>

@optional

/*
 * This method is called when a native ad, placed by the collection view ad placer, will present a modal view controller.
 *
 * @param placer The collection view ad placer that contains the ad displaying the modal.
 */
-(void)nativeAdWillPresentModalForCollectionViewAdPlacer:(MPCollectionViewAdPlacer *)placer;

/*
 * This method is called when a native ad, placed by the collection view ad placer, did dismiss its modal view controller.
 *
 * @param placer The collection view ad placer that contains the ad that dismissed the modal.
 */
-(void)nativeAdDidDismissModalForCollectionViewAdPlacer:(MPCollectionViewAdPlacer *)placer;

/*
 * This method is called when a native ad, placed by the collection view ad placer, will cause the app to background due to user interaction with the ad.
 *
 * @param placer The collection view ad placer that contains the ad causing the app to background.
 */
-(void)nativeAdWillLeaveApplicationFromCollectionViewAdPlacer:(MPCollectionViewAdPlacer *)placer;

@end
