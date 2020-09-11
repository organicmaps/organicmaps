//
//  MPServerAdPositioning.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPAdPositioning.h"

/**
 * The `MPServerAdPositioning` class is a model that allows you to control the positions where
 * native advertisements should appear within a stream. A server positioning object works in
 * conjunction with an ad placer, telling the ad placer that it should retrieve positioning
 * information from the MoPub ad server.
 *
 * Unlike `MPClientAdPositioning`, which represents hard-coded positioning information, a server
 * positioning object offers you the benefit of modifying your ad positions via the MoPub website,
 * without rebuilding your application.
 */

@interface MPServerAdPositioning : MPAdPositioning

/** @name Creating a Server Positioning Object */

/**
 * Creates and returns a server positioning object.
 *
 * When an ad placer is set to use server positioning, it will ask the MoPub ad server for the
 * positions where ads should be inserted into a given stream. These positioning values are
 * configurable on the MoPub website.
 *
 * @return The newly created positioning object.
 *
 * @see MPClientAdPositioning
 */
+ (instancetype)positioning;

@end
