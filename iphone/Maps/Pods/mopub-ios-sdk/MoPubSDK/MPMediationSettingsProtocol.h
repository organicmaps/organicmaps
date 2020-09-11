//
//  MPMediationSettingsProtocol.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

/**
 * The MoPub SDK has a concept of mediation settings that allow you to define objects
 * that allow the application to configure specific settings for your custom event's ad
 * network.
 *
 * Your object can be a global mediation setting that contains settings you deem to be constant
 * across all of your ad network's ads. Ideally this is where you will place settings necessary for
 * your ad network's intialization as well. The global medation setting object should be ready for
 * your custom event by the time you load the ad from your network. Inside your custom event, you can retrieve
 * the global mediation settings by calling `[-globalMediationSettingsForClass:]([MoPub -globalMediationSettingsForClass:])`
 * passing in the class type of your global based mediation settings object.
 *
 * You can also define instance based mediation settings. The application may or may not define
 * a mediation settings object per ad unit ID in their application. This allows ads in different locations
 * to behave differently. The instance based mediation settings object should be available to your custom event
 * by the time you load the ad from your network. Inside your custom event, you can retrieve the instance based
 * mediation settings by calling `[-instanceMediationSettingsForClass:]([MPRewardedVideoCustomEventDelegate -instanceMediationSettingsForClass:])`
 * passing in the class type of your instance based mediation settings object.
 *
 * **Important**: Your custom event must not assume it will receive a global or any instance based mediation settings
 * as the application may choose not to supply any. Your custom event must have a default implementation in the event
 * the application doesn't wish to provide any specific settings.
 */
@protocol MPMediationSettingsProtocol <NSObject>

@end
