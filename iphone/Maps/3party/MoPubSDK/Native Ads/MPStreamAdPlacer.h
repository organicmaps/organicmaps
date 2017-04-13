//
//  MPStreamAdPlacer.h
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "MPClientAdPositioning.h"

@protocol MPStreamAdPlacerDelegate;
@protocol MPNativeAdRendering;
@class MPNativeAdRequestTargeting;
@class MPNativeAd;

/**
 * The `MPStreamAdPlacer` class allows you to retrieve native ads from the MoPub ad server and
 * place them into any custom UI component that represents a stream of content. It does not actually
 * present or insert any ads on its own; you must provide a delegate conforming to the
 * `MPStreamAdPlacerDelegate` protocol to handle ad insertions.
 *
 * @warning **Note:** If you are inserting ads into a `UITableView` or `UICollectionView`, you
 * should first consider whether the `UITableViewAdPlacer` or `UICollectionViewAdPlacer` classes are
 * sufficient for your use case before choosing to use this class.
 *
 * @discussion Your app's first responsibility when creating a stream ad placer is to communicate
 * the state of your stream. Specifically, you must provide it with the count of the
 * original content items in your stream using -setItemCount:forSection:, so that it can determine
 * where and how many ads should appear. Additionally, you must also make sure to notify the ad
 * placer of any insertions, deletions, or rearrangement of content items or sections.
 *
 * Use the -loadAdsForAdUnitID: method to tell the stream ad placer to begin retrieving ads. In
 * order to optimize performance, this call may not immediately result in the ad placer asking its
 * delegate to insert any ads. Instead, the ad placer decides whether to insert ads by determining
 * what content items are currently visible. This means that your delegate may be intermittently
 * informed about new insertions, and is meant to minimize situations in which ads are requested for
 * positions in the stream that have a low likelihood of visibility.
 *
 * ### Responding to Insertions and Rendering Ads
 *
 * Your delegate should respond to insertion callbacks by updating your stream's data source so
 * that it knows to render an ad (rather than an original content item) at the given index path.
 * Note that the implementation may vary depending on the design of your data source.
 *
 * Use -renderAdAtIndexPath:inView: to populate a view with the contents of an ad.
 */

@interface MPStreamAdPlacer : NSObject

/**
 * An array of `NSIndexPath` objects representing the positions of items that are currently visible
 * on the screen.
 *
 * The stream ad placer uses the contents of this array to determine where ads should be inserted.
 * It calculates an on-screen range and uses a small look-ahead to place ads where the user is
 * likely to view them.
 */
@property (nonatomic, strong) NSArray *visibleIndexPaths;

@property (nonatomic, readonly) NSArray *rendererConfigurations;
@property (nonatomic, weak) UIViewController *viewController;
@property (nonatomic, weak) id<MPStreamAdPlacerDelegate> delegate;
@property (nonatomic, readonly, copy) MPAdPositioning *adPositioning;

/**
 * Creates and returns a new ad placer that can display ads in a stream.
 *
 * @param controller The view controller which should be used to present modal content.
 * @param positioning The positioning object that specifies where ads should be shown in the stream.
 * @param rendererConfigurations An array of MPNativeAdRendererConfiguration objects that control how
 * the native ad is rendered. You should pass in configurations that can render any ad type that
 * may be displayed for the given ad unit.
 *
 */
+ (instancetype)placerWithViewController:(UIViewController *)controller adPositioning:(MPAdPositioning *)positioning rendererConfigurations:(NSArray *)rendererConfigurations;

/**
 * Lets the ad placer know of how many items are in a section. This allows the ad placer
 * to place ads more effectively around its visible range.
 *
 * @param count How many items are in the section.
 * @param section The section that the ad placer is recording the count for.
 */
- (void)setItemCount:(NSUInteger)count forSection:(NSInteger)section;

/**
 * Uses the corresponding rendering class to render content in the view.
 *
 * @param indexPath The index path of the cell you want to render.
 * @param view The view you want to render your contents into.
 */
- (void)renderAdAtIndexPath:(NSIndexPath *)indexPath inView:(UIView *)view;

/**
 * Get the size of the ad at the index path.
 *
 * @param indexPath Retrieve the size at indexPath.
 * @param maxWidth The maximum acceptable width for the view.
 *
 * @return The size of the ad at indexPath.
 */
- (CGSize)sizeForAdAtIndexPath:(NSIndexPath *)indexPath withMaximumWidth:(CGFloat)maxWidth;

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

/**
 * Returns whether an ad is ready to be displayed at the indexPath.
 *
 * @param indexPath The index path to examine for ad readiness.
 */
- (BOOL)isAdAtIndexPath:(NSIndexPath *)indexPath;

/**
 * Returns the number of items in the given section of the stream, including any ads that have been
 * inserted.
 *
 * @param numberOfItems The number of content items.
 * @param section The section the method will retrieve the adjusted number of items for.
 */
- (NSUInteger)adjustedNumberOfItems:(NSUInteger)numberOfItems inSection:(NSUInteger)section;

/**
 * Returns the index path representing the location of an item after accounting for ads that have
 * been inserted into the stream.
 *
 * @param indexPath An index path object identifying the original location of a content item, before
 * any ads have been inserted into the stream.
 */
- (NSIndexPath *)adjustedIndexPathForOriginalIndexPath:(NSIndexPath *)indexPath;

/**
 * Asks for the original position of a content item, given its position in the stream after ads
 * have been inserted.
 *
 * If the specified index path does not identify a content item, but rather an ad, this method
 * will return nil.
 *
 * @param indexPath An index path object identifying an item in the stream, after ads have been
 * inserted.
 */
- (NSIndexPath *)originalIndexPathForAdjustedIndexPath:(NSIndexPath *)indexPath;

/**
 * Returns the index paths representing the locations of items after accounting for ads that have
 * been inserted into the stream.
 *
 * @param indexPaths An array of index path objects each identifying the original location of a
 * content item, before any ads have been inserted into the stream.
 */
- (NSArray *)adjustedIndexPathsForOriginalIndexPaths:(NSArray *)indexPaths;

/**
 * Retrieves the original positions of content items, given their positions in the stream after ads
 * have been inserted.
 *
 * If a specified index path does not identify a content item, but rather an ad, it will not be
 * included in the result.
 *
 * @param indexPaths An array of index path objects each identifying an item in the stream, after
 * ads have been inserted.
 */
- (NSArray *)originalIndexPathsForAdjustedIndexPaths:(NSArray *)indexPaths;

/** @name Notifying the Ad Placer of Content Updates */

/**
 * Tells the ad placer that content items have been inserted at the specified index paths.
 *
 * This method allows the ad placer to adjust its ad positions correctly.
 *
 * @param originalIndexPaths An array of NSIndexPath objects that identify positions where content
 * has been inserted.
 */
- (void)insertItemsAtIndexPaths:(NSArray *)originalIndexPaths;

/**
 * Tells the ad placer that content items have been deleted at the specified index paths.
 *
 * This method allows the ad placer to adjust its ad positions correctly, and remove from the
 * stream if necessary
 *
 * @param originalIndexPaths An array of NSIndexPath objects that identify positions where content
 * has been deleted.
*/
- (void)deleteItemsAtIndexPaths:(NSArray *)originalIndexPaths;

/**
 * Tells the ad placer that a content item has moved from one index path to another.
 *
 * This method allows the ad placer to adjust its ad positions correctly.
 *
 * @param fromIndexPath The index path identifying the original location of the item.
 * @param toIndexPath The destination index path for the item.
 */
- (void)moveItemAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath;

/**
 * Tells the ad placer that sections have been inserted at the specified indices.
 *
 * This method allows the ad placer to adjust its ad positions correctly.
 *
 * @param sections An NSIndexSet that identifies the positions where sections
 * have been inserted.
 */
- (void)insertSections:(NSIndexSet *)sections;

/**
 * Tells the ad placer that sections have been deleted at the specified indices.
 *
 * This method allows the ad placer to adjust its ad positions correctly.
 *
 * @param sections An NSIndexSet that identifies the positions where sections
 * have been deleted.
 */
- (void)deleteSections:(NSIndexSet *)sections;

/**
 * Tells the ad placer that a section has moved from one index to another.
 *
 * This method allows the ad placer to adjust its ad positions correctly.
 *
 * @param section The index identifying the original location of the section.
 * @param newSection The destination index for the section.
 */
- (void)moveSection:(NSInteger)section toSection:(NSInteger)newSection;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@protocol MPStreamAdPlacerDelegate <NSObject>

@optional
- (void)adPlacer:(MPStreamAdPlacer *)adPlacer didLoadAdAtIndexPath:(NSIndexPath *)indexPath;
- (void)adPlacer:(MPStreamAdPlacer *)adPlacer didRemoveAdsAtIndexPaths:(NSArray *)indexPaths;

/*
 * This method is called when a native ad, placed by the stream ad placer, will present a modal view controller.
 *
 * @param placer The stream ad placer that contains the ad displaying the modal.
 */
- (void)nativeAdWillPresentModalForStreamAdPlacer:(MPStreamAdPlacer *)adPlacer;

/*
 * This method is called when a native ad, placed by the stream ad placer, did dismiss its modal view controller.
 *
 * @param placer The stream ad placer that contains the ad that dismissed the modal.
 */
- (void)nativeAdDidDismissModalForStreamAdPlacer:(MPStreamAdPlacer *)adPlacer;

/*
 * This method is called when a native ad, placed by the stream ad placer, will cause the app to background due to user interaction with the ad.
 *
 * @param placer The stream ad placer that contains the ad causing the app to background.
 */
- (void)nativeAdWillLeaveApplicationFromStreamAdPlacer:(MPStreamAdPlacer *)adPlacer;

@end
