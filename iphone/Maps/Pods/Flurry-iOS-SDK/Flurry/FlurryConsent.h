//
//  FlurryConsent.h
//  Flurry
//
//  Created by Ishwarya Iyer on 4/2/18.
//  Copyright © 2018 Flurry Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface FlurryConsent : NSObject

@property (nonatomic, assign, readonly) BOOL isGDPRScope;
@property (nonatomic, strong, readonly) NSDictionary* consentStrings;

/*!
 *  @brief Use this api to initialize the FlurryConsent object.
 *  @since 8.5.0
 *
 *  This api initializes the consent object. This object needs to be registered with the SDK.
 *
 *  @param isGDPRScope             @c YES states that GDPR laws apply
 *                                  The default value is @c NO
 *
 *  @param consentStrings          NSDictionary* <NSString*,NSString*> => <consent sting format , consent string>.
 *
 *  @note  The values for ConsentStrings should be of correct base64url-encoded representation.
 *  @note  ConsentStrings must be provided if "isGDPRScope" is set to YES for the consent information to be valid
 *
 */
- (FlurryConsent*) initWithGDPRScope:(BOOL)isGDPRScope andConsentStrings:(NSDictionary*)consentStrings;


/*!
 *  @brief Use this api to register/update the consent information with the SDK.
 *  @since 8.5.0
 *
 *  This api can be called anytime to pass the consent information to the SDK.
 *
 *  @see FlurrySessionBuilder#withConsent: to register the consent before starting Flurry. \n
 *       FlurryConsent#initWithGDPRScope:andConsentStrings to create the FlurryConsent object
 *
 *  @param consent  The FlurryConsent object which contains the GDPR scope flag and the consent strings
 *
 *  @return indicates if the consent information provided is valid or not
 *
 */
+ (BOOL) updateConsentInformation:(FlurryConsent*)consent;

/*!
 *  @brief  Use this api to get the consent information registered with the SDK.
 *  @since 8.5.0
 *
 *  This object is immutable. To update the consent a new FlurryConsent object must be created and passed on to the SDK
 *
 */
+ (FlurryConsent*) getConsent;

@end

