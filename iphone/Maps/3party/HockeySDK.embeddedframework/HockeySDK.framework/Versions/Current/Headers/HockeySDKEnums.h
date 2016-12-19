//
//  HockeySDKEnums.h
//  HockeySDK
//
//  Created by Lukas Spieß on 08/10/15.
//
//

#ifndef HockeySDK_HockeyEnums_h
#define HockeySDK_HockeyEnums_h

/**
 *  HockeySDK App environment
 */
typedef NS_ENUM(NSInteger, BITEnvironment) {
  /**
   *  App has been downloaded from the AppStore
   */
  BITEnvironmentAppStore = 0,
  /**
   *  App has been downloaded from TestFlight
   */
  BITEnvironmentTestFlight = 1,
  /**
   *  App has been installed by some other mechanism.
   *  This could be Ad-Hoc, Enterprise, etc.
   */
  BITEnvironmentOther = 99
};

/**
 *  HockeySDK Crash Reporter error domain
 */
typedef NS_ENUM (NSInteger, BITCrashErrorReason) {
  /**
   *  Unknown error
   */
  BITCrashErrorUnknown,
  /**
   *  API Server rejected app version
   */
  BITCrashAPIAppVersionRejected,
  /**
   *  API Server returned empty response
   */
  BITCrashAPIReceivedEmptyResponse,
  /**
   *  Connection error with status code
   */
  BITCrashAPIErrorWithStatusCode
};

/**
 *  HockeySDK Update error domain
 */
typedef NS_ENUM (NSInteger, BITUpdateErrorReason) {
  /**
   *  Unknown error
   */
  BITUpdateErrorUnknown,
  /**
   *  API Server returned invalid status
   */
  BITUpdateAPIServerReturnedInvalidStatus,
  /**
   *  API Server returned invalid data
   */
  BITUpdateAPIServerReturnedInvalidData,
  /**
   *  API Server returned empty response
   */
  BITUpdateAPIServerReturnedEmptyResponse,
  /**
   *  Authorization secret missing
   */
  BITUpdateAPIClientAuthorizationMissingSecret,
  /**
   *  No internet connection
   */
  BITUpdateAPIClientCannotCreateConnection
};

/**
 *  HockeySDK Feedback error domain
 */
typedef NS_ENUM(NSInteger, BITFeedbackErrorReason) {
  /**
   *  Unknown error
   */
  BITFeedbackErrorUnknown,
  /**
   *  API Server returned invalid status
   */
  BITFeedbackAPIServerReturnedInvalidStatus,
  /**
   *  API Server returned invalid data
   */
  BITFeedbackAPIServerReturnedInvalidData,
  /**
   *  API Server returned empty response
   */
  BITFeedbackAPIServerReturnedEmptyResponse,
  /**
   *  Authorization secret missing
   */
  BITFeedbackAPIClientAuthorizationMissingSecret,
  /**
   *  No internet connection
   */
  BITFeedbackAPIClientCannotCreateConnection
};

/**
 *  HockeySDK Authenticator error domain
 */
typedef NS_ENUM(NSInteger, BITAuthenticatorReason) {
  /**
   *  Unknown error
   */
  BITAuthenticatorErrorUnknown,
  /**
   *  Network error
   */
  BITAuthenticatorNetworkError,

  /**
   *  API Server returned invalid response
   */
  BITAuthenticatorAPIServerReturnedInvalidResponse,
  /**
   *  Not Authorized
   */
  BITAuthenticatorNotAuthorized,
  /**
   *  Unknown Application ID (configuration error)
   */
  BITAuthenticatorUnknownApplicationID,
  /**
   *  Authorization secret missing
   */
  BITAuthenticatorAuthorizationSecretMissing,
  /**
   *  Not yet identified
   */
  BITAuthenticatorNotIdentified,
};

/**
 *  HockeySDK global error domain
 */
typedef NS_ENUM(NSInteger, BITHockeyErrorReason) {
  /**
   *  Unknown error
   */
  BITHockeyErrorUnknown
};

#endif /* HockeySDK_HockeyEnums_h */
