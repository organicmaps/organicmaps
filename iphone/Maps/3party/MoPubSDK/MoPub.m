//
//  MoPub.m
//  MoPub
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import "MoPub.h"
#import "MPAdvancedBiddingManager.h"
#import "MPConsentManager.h"
#import "MPConstants.h"
#import "MPCoreInstanceProvider.h"
#import "MPGeolocationProvider.h"
#import "MPLogging.h"
#import "MPMediationManager.h"
#import "MPRewardedVideo.h"
#import "MPRewardedVideoCustomEvent+Caching.h"
#import "MPIdentityProvider.h"
#import "MPWebView.h"
#import "MOPUBExperimentProvider.h"
#import "MPViewabilityTracker.h"
#import "MPAdConversionTracker.h"
#import "MPConsentManager.h"
#import "MPConsentChangedNotification.h"

@interface MoPub ()

@property (nonatomic, strong) NSArray *globalMediationSettings;

@property (nonatomic, assign, readwrite) BOOL isSdkInitialized;

@end

@implementation MoPub

+ (MoPub *)sharedInstance
{
    static MoPub *sharedInstance = nil;
    static dispatch_once_t initOnceToken;
    dispatch_once(&initOnceToken, ^{
        sharedInstance = [[MoPub alloc] init];
    });
    return sharedInstance;
}

- (instancetype)init
{
    if (self = [super init]) {
        // Processing personal data if a user is in GDPR region.
        [self handlePersonalData];
    }
    return self;
}

- (void)setLocationUpdatesEnabled:(BOOL)locationUpdatesEnabled
{
    [[[MPCoreInstanceProvider sharedProvider] sharedMPGeolocationProvider] setLocationUpdatesEnabled:locationUpdatesEnabled];
}

- (BOOL)locationUpdatesEnabled
{
    return [[MPCoreInstanceProvider sharedProvider] sharedMPGeolocationProvider].locationUpdatesEnabled;
}

- (void)setFrequencyCappingIdUsageEnabled:(BOOL)frequencyCappingIdUsageEnabled
{
    [MPIdentityProvider setFrequencyCappingIdUsageEnabled:frequencyCappingIdUsageEnabled];
}

- (void)setForceWKWebView:(BOOL)forceWKWebView
{
    [MPWebView forceWKWebView:forceWKWebView];
}

- (BOOL)forceWKWebView
{
    return [MPWebView isForceWKWebView];
}

- (void)setLogLevel:(MPLogLevel)level
{
    MPLogSetLevel(level);
}

- (MPLogLevel)logLevel
{
    return MPLogGetLevel();
}

- (void)setEnableAdvancedBidding:(BOOL)enableAdvancedBidding
{
    [MPAdvancedBiddingManager sharedManager].advancedBiddingEnabled = enableAdvancedBidding;
}

- (BOOL)enableAdvancedBidding
{
    return [MPAdvancedBiddingManager sharedManager].advancedBiddingEnabled;
}

- (void)setClickthroughDisplayAgentType:(MOPUBDisplayAgentType)displayAgentType
{
    [MOPUBExperimentProvider setDisplayAgentType:displayAgentType];
}

- (BOOL)frequencyCappingIdUsageEnabled
{
    return [MPIdentityProvider frequencyCappingIdUsageEnabled];
}

// Keep -version and -bundleIdentifier methods around for Fabric backwards compatibility.
- (NSString *)version
{
    return MP_SDK_VERSION;
}

- (NSString *)bundleIdentifier
{
    return MP_BUNDLE_IDENTIFIER;
}

- (void)initializeSdkWithConfiguration:(MPMoPubConfiguration *)configuration
                            completion:(void(^_Nullable)(void))completionBlock
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        [self setSdkWithConfiguration:configuration completion:completionBlock];
    });
}

- (void)setSdkWithConfiguration:(MPMoPubConfiguration *)configuration
                     completion:(void(^_Nullable)(void))completionBlock
{
    @synchronized (self) {
        // Store the global mediation settings
        self.globalMediationSettings = configuration.globalMediationSettings;

        // Create a dispatch group to synchronize mutliple asynchronous tasks.
        dispatch_group_t initializationGroup = dispatch_group_create();

        // Configure the consent manager and synchronize regardless of the result
        // of `checkForDoNotTrackAndTransition`.
        dispatch_group_enter(initializationGroup);
        MPConsentManager.sharedManager.adUnitIdUsedForConsent = configuration.adUnitIdForAppInitialization;
        [MPConsentManager.sharedManager checkForDoNotTrackAndTransition];
        [MPConsentManager.sharedManager synchronizeConsentWithCompletion:^(NSError * _Nullable error) {
            dispatch_group_leave(initializationGroup);
        }];

        // Configure mediated network SDKs
        dispatch_group_enter(initializationGroup);
        NSArray<Class<MPMediationSdkInitializable>> * mediatedNetworks = configuration.mediatedNetworks;
        [MPMediationManager.sharedManager initializeMediatedNetworks:mediatedNetworks completion:^(NSError * _Nullable error) {
            dispatch_group_leave(initializationGroup);
        }];

        // Configure advanced bidders
        dispatch_group_enter(initializationGroup);
        [MPAdvancedBiddingManager.sharedManager initializeBidders:configuration.advancedBidders complete:^{
            dispatch_group_leave(initializationGroup);
        }];

        // Once all of the asynchronous tasks have completed, notify the
        // completion handler.
        dispatch_group_notify(initializationGroup, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
            self.isSdkInitialized = YES;
            if (completionBlock) {
                completionBlock();
            }
        });
    }
}

- (void)handlePersonalData
{
    [[MPConsentManager sharedManager] checkForIfaChange];
}

- (id<MPMediationSettingsProtocol>)globalMediationSettingsForClass:(Class)aClass
{
    NSArray *mediationSettingsCollection = self.globalMediationSettings;

    for (id<MPMediationSettingsProtocol> settings in mediationSettingsCollection) {
        if ([settings isKindOfClass:aClass]) {
            return settings;
        }
    }

    return nil;
}

- (void)disableViewability:(MPViewabilityOption)vendors
{
    [MPViewabilityTracker disableViewability:vendors];
}

@end

@implementation MoPub (Mediation)

- (NSArray<Class<MPMediationSdkInitializable>> * _Nullable)allCachedNetworks {
    return [MPMediationManager.sharedManager allCachedNetworks];
}

- (void)clearCachedNetworks {
    return [MPMediationManager.sharedManager clearCache];
}

@end

@implementation MoPub (Consent)

- (BOOL)shouldShowConsentDialog {
    return [MPConsentManager sharedManager].isConsentNeeded;
}

- (BOOL)canCollectPersonalInfo {
    return [MPConsentManager sharedManager].canCollectPersonalInfo;
}

- (MPBool)isGDPRApplicable {
    return [MPConsentManager sharedManager].isGDPRApplicable;
}

- (MPConsentStatus)currentConsentStatus {
    return [MPConsentManager sharedManager].currentStatus;
}

- (NSString *)currentConsentIabVendorListFormat {
    return [MPConsentManager sharedManager].iabVendorList;
}

- (NSString *)currentConsentPrivacyPolicyVersion {
    return [MPConsentManager sharedManager].privacyPolicyVersion;
}

- (NSString *)currentConsentVendorListVersion {
    return [MPConsentManager sharedManager].vendorListVersion;
}

- (NSString *)previouslyConsentedIabVendorListFormat {
    return [MPConsentManager sharedManager].consentedIabVendorList;
}

- (NSString *)previouslyConsentedPrivacyPolicyVersion {
    return [MPConsentManager sharedManager].consentedPrivacyPolicyVersion;
}

- (NSString *)previouslyConsentedVendorListVersion {
    return [MPConsentManager sharedManager].consentedVendorListVersion;
}

- (void)loadConsentDialogWithCompletion:(void (^)(NSError * _Nullable))completion {
    [[MPConsentManager sharedManager] loadConsentDialogWithCompletion:completion];
}

- (void)showConsentDialogFromViewController:(UIViewController *)viewController completion:(void (^ _Nullable)(void))completion {
    [[MPConsentManager sharedManager] showConsentDialogFromViewController:viewController completion:completion];
}

- (BOOL)isConsentDialogReady {
    return [MPConsentManager sharedManager].isConsentDialogLoaded;
}

- (void)revokeConsent {
    [[MPConsentManager sharedManager] revokeConsent];
}

- (void)grantConsent {
    [[MPConsentManager sharedManager] grantConsent];
}

- (NSURL *)currentConsentPrivacyPolicyUrl {
    return [[MPConsentManager sharedManager] privacyPolicyUrl];
}

- (NSURL *)currentConsentPrivacyPolicyUrlWithISOLanguageCode:(NSString *)isoLanguageCode {
    return [[MPConsentManager sharedManager] privacyPolicyUrlWithISOLanguageCode:isoLanguageCode];
}

- (NSURL *)currentConsentVendorListUrl {
    return [[MPConsentManager sharedManager] vendorListUrl];
}

- (NSURL *)currentConsentVendorListUrlWithISOLanguageCode:(NSString *)isoLanguageCode {
    return [[MPConsentManager sharedManager] vendorListUrlWithISOLanguageCode:isoLanguageCode];
}

@end
