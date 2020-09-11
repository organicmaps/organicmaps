//
//  MPMediationManager.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPMediationManager.h"
#import "MPError.h"
#import "MPLogging.h"
#import "NSBundle+MPAdditions.h"

// Macros for dispatching asynchronously to the main queue
#define mp_safe_block(block, ...) block ? block(__VA_ARGS__) : nil

/**
 Key of the @c NSUserDefaults entry for the network initialization cache.
 */
static NSString * const kNetworkSDKInitializationParametersKey = @"com.mopub.mopub-ios-sdk.network-init-info";

// File name and extension of the certified adapter information providers file.
// This should correspond to `MPAdapters.plist` in the Resources directory.
static NSString * kAdaptersFile     = @"MPAdapters";
static NSString * kAdaptersFileType = @"plist";

// Ad request JSON payload keys.
static NSString const * kAdapterOptionsKey    = @"options";
static NSString const * kAdapterVersionKey    = @"adapter_version";
static NSString const * kNetworkSdkVersionKey = @"sdk_version";
static NSString const * kTokenKey             = @"token";

@interface MPMediationManager()
/**
 Dictionary of all instantiated adapter information providers.
 */
@property (nonatomic, strong, readwrite) NSMutableDictionary<NSString *, id<MPAdapterConfiguration>> * adapters;

/**
 All certified adapter information classes that exist within the current runtime.
 */
@property (nonatomic, strong, readonly) NSSet<Class<MPAdapterConfiguration>> * certifiedAdapterClasses;

/**
 Flag indicating if an initialization is already in progress.
 */
@property (nonatomic, assign) BOOL isInitializing;

/**
 Initialization queue.
 */
@property (nonatomic, strong) dispatch_queue_t queue;
@end

@implementation MPMediationManager

#pragma mark - Initialization

+ (instancetype)sharedManager {
    static MPMediationManager * sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[MPMediationManager alloc] init];
    });

    return sharedInstance;
}

- (instancetype)init {
    if (self = [super init]) {
        _adapters = [NSMutableDictionary dictionary];
        _certifiedAdapterClasses = MPMediationManager.certifiedAdapterInformationProviderClasses;
        _isInitializing = NO;
        _queue = dispatch_queue_create("Mediated Adapter Initialization Queue", DISPATCH_QUEUE_SERIAL);
    }

    return self;
}

- (void)initializeWithAdditionalProviders:(NSArray<Class<MPAdapterConfiguration>> *)providers
                           configurations:(NSDictionary<NSString *, NSDictionary<NSString *, id> *> *)configurations
                           requestOptions:(NSDictionary<NSString *, NSDictionary<NSString *, NSString *> *> * _Nullable)options
                                 complete:(MPMediationInitializationCompletionBlock)complete {
    // Initialization is already in progress; error out.
    if (self.isInitializing) {
        mp_safe_block(complete, NSError.sdkInitializationInProgress, nil);
        return;
    }

    // Start the initialization process
    self.isInitializing = YES;

    // Combines the additional providers with the existing set of certified adapter
    // information providers (if needed).
    NSSet * classesToInitialize = (providers != nil ? [self.certifiedAdapterClasses setByAddingObjectsFromArray:providers] : self.certifiedAdapterClasses);

    // There are no adapter information providers to initialize. Do nothing.
    if (classesToInitialize.count == 0) {
        self.isInitializing = NO;
        mp_safe_block(complete, nil, nil);
        return;
    }

    // Holds all of the successfully initialized adapter information providers.
    NSMutableDictionary<NSString *, id<MPAdapterConfiguration>> * initializedAdapters = [NSMutableDictionary dictionaryWithCapacity:classesToInitialize.count];

    // Attempt to instantiate and initialize each adapter information provider.
    // If a network has an invalid `moPubNetworkName` or has already been initialized,
    // that network will be skipped.
    [classesToInitialize enumerateObjectsUsingBlock:^(Class<MPAdapterConfiguration> adapterInfoProviderClass, BOOL * _Nonnull stop) {
        // Create an instance of the adapter configuration
        id<MPAdapterConfiguration> adapterInfoProvider = (id<MPAdapterConfiguration>)[[[adapterInfoProviderClass class] alloc] init];
        NSString * network = adapterInfoProvider.moPubNetworkName;

        // Verify that the adapter information provider has a MoPub network name and that it's
        // not already created.
        if (network.length == 0 || initializedAdapters[network] != nil) {
            return;
        }

        // Retrieve the full set of initialization parameters.
        NSDictionary * initializationParams = [self parametersForAdapter:adapterInfoProvider overrideConfiguration:configurations[NSStringFromClass(adapterInfoProviderClass)]];

        // Populate the request options (if any)
        [adapterInfoProvider addMoPubRequestOptions:options[NSStringFromClass(adapterInfoProviderClass)]];

        // Queue up the adapter's underlying SDK initialization.
        dispatch_async(self.queue, ^{
            [adapterInfoProvider initializeNetworkWithConfiguration:initializationParams complete:^(NSError * error) {
                // Log adapter initialization error.
                if (error != nil) {
                    NSString * logPrefix = [NSString stringWithFormat:@"Adapter %@ encountered an error during initialization", NSStringFromClass(adapterInfoProviderClass)];
                    MPLogEvent([MPLogEvent error:error message:logPrefix]);
                }
            }];
        });

        // Adapter initialization is complete.
        initializedAdapters[network] = adapterInfoProvider;
    }];

    // Set the initialized adapters and update the internal state.
    self.adapters = initializedAdapters;
    self.isInitializing = NO;
    mp_safe_block(complete, nil, initializedAdapters.allValues);
}

#pragma mark - Computed Properties

- (NSDictionary<NSString *, NSDictionary *> *)adRequestPayload {
    // There are no initialized adapter information providers; send nothing.
    if (self.adapters.count == 0) {
        return nil;
    }

    // Build the JSON payload.
    NSMutableDictionary * payload = [NSMutableDictionary dictionaryWithCapacity:self.adapters.count];
    [self.adapters enumerateKeysAndObjectsUsingBlock:^(NSString * _Nonnull key, id<MPAdapterConfiguration> _Nonnull adapter, BOOL * _Nonnull stop) {
        NSMutableDictionary * adapterPayload = [NSMutableDictionary dictionary];
        adapterPayload[kAdapterOptionsKey] = adapter.moPubRequestOptions;
        adapterPayload[kAdapterVersionKey] = adapter.adapterVersion;
        adapterPayload[kNetworkSdkVersionKey] = adapter.networkSdkVersion;
        // Advanced Bidding tokens have been disabled from the adapter payload
        // since we are currently sending the tokens in the former `abt` field,
        // and do not want to send the tokens twice.
        // Once the `abt` field has been deprecated, this token should be re-enabled.
        //adapterPayload[kTokenKey] = adapter.biddingToken;

        payload[key] = adapterPayload;
    }];

    return payload;
}

#pragma mark - Certified Adapter Information Providers

/**
 Attempts to retrieve @c MPAdapters.plist from the current bundle's resources.
 @return The file path if available; otherwise @c nil.
 */
+ (NSString *)adapterInformationProvidersFilePath {
    // Retrieve the plist containing the default adapter information provider class names.
    NSBundle * parentBundle = [NSBundle resourceBundleForClass:self.class];
    NSString * filepath = [parentBundle pathForResource:kAdaptersFile ofType:kAdaptersFileType];
    return filepath;
}

/**
 Retrieves the certified adapter information classes that exist within the
 current runtime.
 @return List of certified adapter information classes that exist in the runtime.
 */
+ (NSSet<Class<MPAdapterConfiguration>> * _Nonnull)certifiedAdapterInformationProviderClasses {
    // Certified adapters file not present. Do not continue.
    NSString * filepath = MPMediationManager.adapterInformationProvidersFilePath;
    if (filepath == nil) {
        MPLogInfo(@"Could not find MPAdapters.plist.");
        return [NSSet set];
    }

    // Try to retrieve the class for each certified adapter
    NSArray<NSString *> * adapterClassNames = [NSArray arrayWithContentsOfFile:filepath];
    NSMutableSet<Class<MPAdapterConfiguration>> * adapterInfoClasses = [NSMutableSet setWithCapacity:adapterClassNames.count];
    [adapterClassNames enumerateObjectsUsingBlock:^(NSString * _Nonnull className, NSUInteger idx, BOOL * _Nonnull stop) {
        // Adapter information provider is valid since we can retrieve the class and it conforms
        // to the `MPAdapterConfiguration` protocol.
        Class adapterClass = NSClassFromString(className);
        if (adapterClass != Nil && [adapterClass conformsToProtocol:@protocol(MPAdapterConfiguration)]) {
            [adapterInfoClasses addObject:adapterClass];
        }
    }];

    return adapterInfoClasses;
}

/**
 Combines cached initialization parameters with override parameters.
 @param adapter Adapter information provider that will be populated.
 @param configuration Externally-specified initialization parameters.
 @return The combined initialization parameters with any @c moPubRequestOptions removed. In the event that
 there are no parameters, @c nil is returned.
 */
- (NSDictionary<NSString *, id> *)parametersForAdapter:(id<MPAdapterConfiguration>)adapter
                                 overrideConfiguration:(NSDictionary<NSString *, id> * _Nullable)configuration {
    // Retrieve the adapter's cached initialization parameters and inputted initialization parameters.
    // Combine the two dictionaries, giving preference to the publisher-inputted parameters.
    NSDictionary * cachedParameters = [self cachedInitializationParametersForNetwork:adapter.class];

    NSMutableDictionary * initializationParams = (cachedParameters != nil ? [NSMutableDictionary dictionaryWithDictionary:cachedParameters] : [NSMutableDictionary dictionary]);
    if (configuration != nil) {
        [initializationParams addEntriesFromDictionary:configuration];
    }

    return (initializationParams.count > 0 ? initializationParams : nil);
}

#pragma mark - Cache

- (void)setCachedInitializationParameters:(NSDictionary * _Nullable)params forNetwork:(Class<MPAdapterConfiguration> _Nonnull)networkClass {
    // Empty network names and parameters are invalid.
    NSString * network = NSStringFromClass(networkClass);
    if (network.length == 0 || params == nil) {
        return;
    }

    @synchronized (self) {
        NSMutableDictionary * cachedParameters = [[[NSUserDefaults standardUserDefaults] objectForKey:kNetworkSDKInitializationParametersKey] mutableCopy];
        if (cachedParameters == nil) {
            cachedParameters = [NSMutableDictionary dictionaryWithCapacity:1];
        }

        cachedParameters[network] = params;
        [[NSUserDefaults standardUserDefaults] setObject:cachedParameters forKey:kNetworkSDKInitializationParametersKey];
        [[NSUserDefaults standardUserDefaults] synchronize];

        MPLogInfo(@"Cached SDK initialization parameters for %@:\n%@", network, params);
    }
}

- (NSDictionary * _Nullable)cachedInitializationParametersForNetwork:(Class<MPAdapterConfiguration>)networkClass {
    // Empty network names are invalid.
    NSString * network = NSStringFromClass(networkClass);
    if (network.length == 0) {
        return nil;
    }

    @synchronized (self) {
        NSDictionary *cachedParameters = [[NSUserDefaults standardUserDefaults] objectForKey:kNetworkSDKInitializationParametersKey];
        return [cachedParameters objectForKey:network];
    }
}

- (void)clearCache {
    @synchronized (self) {
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:kNetworkSDKInitializationParametersKey];
        [[NSUserDefaults standardUserDefaults] synchronize];

        MPLogDebug(@"Cleared cached SDK initialization parameters");
    }
}

#pragma mark - Advanced Bidding

- (NSDictionary<NSString *, NSDictionary *> *)advancedBiddingTokens {
    // No adapters.
    if (self.adapters.count == 0) {
        return nil;
    }

    // Generate the JSON dictionary for all participating bidders.
    NSMutableDictionary * tokens = [NSMutableDictionary dictionary];
    [self.adapters enumerateKeysAndObjectsUsingBlock:^(NSString * _Nonnull networkName, id<MPAdapterConfiguration>  _Nonnull adapter, BOOL * _Nonnull stop) {
        if (adapter.biddingToken != nil) {
            tokens[networkName] = @{ kTokenKey: adapter.biddingToken };
        }
    }];

    return tokens;
}

@end
