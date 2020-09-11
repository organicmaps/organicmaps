//
//  MPClientAdPositioning.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>
#import "MPAdPositioning.h"

/**
 * The `MPClientAdPositioning` class is a model that allows you to control the positions where
 * native advertisements should appear within a stream. A positioning object works in conjunction
 * with an ad placer, giving the ad placer the information it needs to configure the positions and
 * frequency of ads. You can specify that ads should appear at fixed index paths and/or at equally
 * spaced intervals throughout your content.
 *
 * Unlike with `MPServerAdPositioning`, which tells an ad placer to obtain its positioning
 * information from the MoPub ad server, client ad positioning does not allow you to control your ad
 * positions via the MoPub website.
 */

@interface MPClientAdPositioning : MPAdPositioning

/** @name Creating a Client Positioning Object */

/**
 * Creates and returns an empty positioning object. In order for ads to display in a stream, the
 * positioning object must either have at least one fixed position or have repeating positions
 * enabled.
 *
 * @return The newly created positioning object.
 */
+ (instancetype)positioning;

/**
 * Tells the positioning object that an ad should be placed at the specified position.
 *
 * Positions are passed in as absolute index paths within a stream. For example, if you place an
 * ad in a table view at a fixed index path with row 1, an ad will appear in row 1, which may shift
 * other content items to higher row indexes.
 *
 * Note: this method uses `NSIndexPath` objects to accommodate streams with multiple sections. If
 * your stream does not contain multiple sections, you should pass in index paths with a section
 * value of 0.
 *
 * @param indexPath An index path representing a position for an ad.
 */
- (void)addFixedIndexPath:(NSIndexPath *)indexPath;

/**
 * Tells the positioning object that ads should be displayed evenly throughout a stream using the
 * specified interval.
 *
 * Repeating ads will only appear within a single section. If the receiver has fixed positions,
 * the sequence of repeating ads will start to appear following the last registered fixed position.
 * If the receiver does not have any fixed positions, ads will appear regularly starting at
 * `interval`, within the first section.
 *
 * @param interval The frequency at which to display ads. This must be a value greater than 1.
 */
- (void)enableRepeatingPositionsWithInterval:(NSUInteger)interval;

@end
