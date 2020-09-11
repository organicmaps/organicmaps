//
//  MPConsentManager.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <UIKit/UIKit.h>
#import "MPBool.h"
#import "MPConsentStatus.h"
#import "MPConsentDialogViewController.h"

@interface MPConsentManager : NSObject

/**
 Ad unit ID sent to Ad Server as a proxy for the MoPub app ID. If a known
 good adunit ID is already cached, setting this will have no effect.
 @remark This should only be set by SDK initialization and must be non-nil.
 */
@property (nonatomic, strong, nonnull) NSString * adUnitIdUsedForConsent;

/**
 Sets @c self.adUnitIdUsedForConsent, and caches to disk if @c isKnownGood is set to @c YES.
 No-op if a known good adunit is already cached to disk.
 @remark @c isKnownGood should only be set to @c YES when the adunit ID has been verified with the server
 */
- (void)setAdUnitIdUsedForConsent:(NSString * _Nonnull)adUnitIdUsedForConsent isKnownGood:(BOOL)isKnownGood;

/**
 Clears @c self.adUnitIdUsedForConsent as well as the backing cache.
 */
- (void)clearAdUnitIdUsedForConsent;

/**
 This API can be used if you want to allow supported SDK networks to collect user information on the basis of legitimate interest. The default value is @c NO.
 */
@property (nonatomic, assign) BOOL allowLegitimateInterest;

/**
 Flag indicating that personally identifiable information can be collected.
 */
@property (nonatomic, readonly) BOOL canCollectPersonalInfo;

/**
 Flag indicating that consent needs to be acquired (or reacquired) by the user, and
 that the consent dialog may need to be shown.
 */
@property (nonatomic, readonly) BOOL isConsentNeeded;

/**
 Flag indicating that GDPR applicability was forced and the transition should be
 communicated back to the server. This will only persist in memory.
 */
@property (nonatomic, readonly) BOOL isForcedGDPRAppliesTransition;

/**
 Retrieves the current language code.
 */
@property (nonatomic, copy, readonly, nonnull) NSString * currentLanguageCode;

/**
 * Singleton instance of the manager.
 */
+ (MPConsentManager * _Nonnull)sharedManager;

/**
 Allows a whitelisted publisher to grant consent on the user's behalf.
 */
- (void)grantConsent;

/**
 Allows a publisher to explicitly deny or revoke consent on the user's behalf.
 */
- (void)revokeConsent;

/**
 Checks if there is a transition from a "do not track" state to an "allowed to track" state, or from
 an "allowed to track" state to a "do not track" state. If detected, the appropriate consent
 status state change will occur locally and trigger the @c kMPConsentChangedNotification.
 @remark This is a local update only and will require a seperate Ad Server synchronization.
 @return @c YES if a transition occurred; @c NO otherwise.
 */
- (BOOL)checkForDoNotTrackAndTransition;

/**
 This method takes in parameters (expected to be derived from a server response) which tell the SDK to force consent
 status to explicit no or unknown. Explicit no takes priority over unknown. It also takes in a consent change reason,
 and a BOOL to tell it whether to cache and broadcast.
 */
- (void)forceStatusShouldForceExplicitNo:(BOOL)shouldForceExplicitNo
                 shouldInvalidateConsent:(BOOL)shouldInvalidateConsent
                  shouldReacquireConsent:(BOOL)shouldReacquireConsent
            shouldForceGDPRApplicability:(BOOL)shouldForceGDPRApplies
                     consentChangeReason:(NSString * _Nullable)consentChangeReason
                 shouldBroadcast:(BOOL)shouldBroadcast;

/**
 Synchronizes the current state with Ad Server and makes any state adjustments
 based upon the response.
 @param completion Synchronization completion block.
 */
- (void)synchronizeConsentWithCompletion:(void (^ _Nonnull)(NSError * _Nullable error))completion;

/**
 `YES` if a consent dialog is currently loaded; `NO` otherwise.
 */
@property (nonatomic, readonly) BOOL isConsentDialogLoaded;

/**
 Starts loading a consent dialog asynchronously. Calls `completion` when done with an NSError if not successful.
 Calls `completion` immediately if a `consentDialog` is already loaded.
 */
- (void)loadConsentDialogWithCompletion:(void (^ _Nullable)(NSError * _Nullable error))completion;

/**
 If a consent dialog is loaded, this method will present it modally from the given `viewController`. If no consent
 dialog is loaded this method will do nothing. `completion` is called upon successful presentation; it is not called otherwise.
 */
- (void)showConsentDialogFromViewController:(UIViewController * _Nonnull)viewController
                                    didShow:(void (^ _Nullable)(void))didShow
                                 didDismiss:(void (^ _Nullable)(void))didDismiss;

@end

@interface MPConsentManager (State)

/**
 IAB vendor list that has been consented to.
 */
@property (nonatomic, copy, readonly, nullable) NSString * consentedIabVendorList;

/**
 MoPub privacy policy version that has been consented to.
 */
@property (nonatomic, copy, readonly, nullable) NSString * consentedPrivacyPolicyVersion;

/**
 Vendor list version that has been consented to.
 */
@property (nonatomic, copy, readonly, nullable) NSString * consentedVendorListVersion;

/**
 Current consent status.
 */
@property (nonatomic, readonly) MPConsentStatus currentStatus;

/**
 Server extras (reserved for future use).
 */
@property (nonatomic, copy, readonly, nullable) NSString * extras;

/**
 Current IAB format vendor list.
 */
@property (nonatomic, copy, readonly, nullable) NSString * iabVendorList;

/**
 Current IAB format vendor list hash.
 */
@property (nonatomic, copy, readonly, nullable) NSString * iabVendorListHash;

/**
 IFA used for consent purposes only.
 */
@property (nonatomic, copy, readonly, nullable) NSString * ifaForConsent;

/**
 Flag indicating if GDPR is applicable to the user.
 */
@property (nonatomic, readonly) MPBool isGDPRApplicable;

/**
 Allows a publisher to force @c isGDPRApplicable to @c YES. When this is set to @c YES, @c isGDPRApplicable will always
 be @c MPBoolYes. This property is disk-backed, so its value will persist between app sessions once it has been set.
 When set back to @c NO, the value of @c isGDPRApplicable determined at first app session will apply.
 */
@property (nonatomic, assign) BOOL forceIsGDPRApplicable;

/**
 Flag indicating that the app is whitelisted for non-user-initiated consent changes.
 */
@property (nonatomic, readonly) BOOL isWhitelisted;

/**
 Optional description of why the consent was changed to the current value.
 */
@property (nonatomic, copy, readonly, nullable) NSString * lastChangedReason;

/**
 Timestamp of the current consent status in milliseconds. If this value is zero,
 consent status has never changed.
 */
@property (nonatomic, readonly) NSTimeInterval lastChangedTimestampInMilliseconds;

/**
 Consent status that was last synchronized with the server.
 */
@property (nonatomic, copy, readonly, nullable) NSString * lastSynchronizedStatus;

/**
 URL to the MoPub privacy policy in the device's preferred language. If the device's
 preferred language could not be determined, English will be used.
 @return The privacy policy URL for the desired language if successful; @c nil if
 there is no current vendor list.
 */
- (NSURL * _Nullable)privacyPolicyUrl;

/**
 URL to the MoPub privacy policy in the language of choice.
 @param isoLanguageCode ISO 630-1 language code
 @return The privacy policy URL for the desired language if successful; @c nil if the
 language code is invalid or if there is no current vendor list.
 */
- (NSURL * _Nullable)privacyPolicyUrlWithISOLanguageCode:(NSString * _Nonnull)isoLanguageCode;

/**
 Current version of MoPub’s privacy policy.
 */
@property (nonatomic, copy, readonly, nullable) NSString * privacyPolicyVersion;

/**
 The maximum frequency in seconds that the SDK is allowed to synchronize consent
 information.
 */
@property (nonatomic, readonly) NSTimeInterval syncFrequency;

/**
 Current vendor list URL in the device's preferred language. If the device's
 preferred language could not be determined, English will be used.
 @return The vendor list URL for the desired language if successful; @c nil if
 there is no current vendor list.
 */
- (NSURL * _Nullable)vendorListUrl;

/**
 Current vendor list URL in the language of choice.
 @param isoLanguageCode ISO 630-1 language code
 @return The vendor list URL for the desired language if successful; @c nil if the
 language code is invalid or if there is no current vendor list.
 */
- (NSURL * _Nullable)vendorListUrlWithISOLanguageCode:(NSString * _Nonnull)isoLanguageCode;

/**
 Current version of the vendor list.
 */
@property (nonatomic, copy, readonly, nullable) NSString * vendorListVersion;

@end

@interface MPConsentManager (PersonalDataHandler)

/**
 * Clean up personal data and add additonal logic for personal data when consent state changes.
 */

- (void)handlePersonalDataOnStateChangeTo:(MPConsentStatus)newStatus fromOldStatus:(MPConsentStatus)oldStatus;

/**
 * Store IFA(IDFA) temporary in NSUserDefault during MoPub initialization or app foregrounding. IFA is only used for removing personal data.
 *
 */
- (void)storeIfa;

/**
 * Remove IFA from NSUserDefault.
 */
- (void)removeIfa;

/**
 * If IFA is changed and the status is transitioning from MPConsentStatusConsented, remove old IFA from NSUserDefault and change status to unknown.
 *
 */
- (void)checkForIfaChange;

/**
 * App conversion request will only be fired when MoPub obtains consent.
 */
- (void)updateAppConversionTracking;

@end
