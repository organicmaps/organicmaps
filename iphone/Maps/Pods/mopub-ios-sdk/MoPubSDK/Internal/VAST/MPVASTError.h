//
//  MPVASTError.h
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 `MPVASTError` enums are the error codes defined in VAST specification. See the VAST specifications
 at https://iabtechlab.com/standards/vast/.

 The current version of `MPVASTError` adopts VAST version 4.2 error codes as defined in section
 "2.3.6.3 VAST Error Codes Table". VAST error codes are designed to be backward compatible, which
 means this set of error codes is a superset of previous versions of them.

 Note: Only a portion of these error codes are implemented in the MoPub SDK, as some of the error codes
 are deprecated, not applicable, or being too vague for the meaning. Unused error codes should be
 commented out and kept for reference.

 Per VAST spec, at a minimum, error code 900 `MPVASTErrorUndefined` can be used, but a more specific
 error code benefits all parties involved.
 */
typedef NS_ENUM(NSUInteger, MPVASTError) {
    /// XML parsing error.
    MPVASTErrorXMLParseFailure = 100,

    /// VAST schema validation error.
    //MPVASTErrorSchemeValidationError = 101,

    /// VAST version of response not supported.
    //MPVASTErrorUnsupportedVersionOfResponse = 102,

    /// Trafficking error. Media player received an Ad type that it was not expecting and/or cannot play.
    MPVASTErrorCannotPlayMedia = 200,

    /// Media player expecting different linearity.
    //MPVASTErrorMediaPlayerExpectingDifferentLinearity = 201,

    /// Media player expecting different duration.
    //MPVASTErrorMediaPlayerExpectingDifferentDuration = 202,

    /// Media player expecting different size.
    //MPVASTErrorMediaPlayerExpectingDifferentSize = 203,

    /// Ad category was required but not provided.
    //MPVASTErrorRequiredAdCategoryNotProvided = 204,

    /// Inline Category violates Wrapper BlockedAdCategories.
    //MPVASTErrorInlineCategoryViolatesWrapperBlockedAdCategories = 205,

    /// Ad Break shortened. Ad was not served.
    //MPVASTErrorAdBreakShortenedAndWasNotServed = 206,

    /// General Wrapper error.
    //MPVASTErrorGeneralWrapperError = 300,

    /// Timeout of VAST URI provided in Wrapper element, or of VAST URI provided in a subsequent
    /// Wrapper element. (URI was either unavailable or reached a timeout as defined by the media player.)
    //MPVASTErrorURITimeout = 301,

    /// Wrapper limit reached, as defined by the media player. Too many Wrapper responses have been
    /// received with no InLine response.
    MPVASTErrorExceededMaximumWrapperDepth = 302,

    /// No VAST response after one or more Wrappers.
    MPVASTErrorNoVASTResponseAfterOneOrMoreWrappers = 303,

    /// InLine response returned ad unit that failed to result in ad display within defined time limit.
    MPVASTErrorFailedToDisplayAdFromInlineResponse = 304,

    /// General Linear error. Media player is unable to display the Linear Ad.
    //MPVASTErrorMediaPlayerUnableToDisplayLinearAd = 400,

    /// File not found. Unable to find Linear/MediaFile from URI.
    MPVASTErrorUnableToFindLinearAdOrMediaFileFromURI = 401,

    /// Timeout of MediaFile URI.
    MPVASTErrorTimeoutOfMediaFileURI = 402,

    /// Couldn’t find MediaFile that is supported by this media player, based on the attributes of the
    /// MediaFile element.
    //MPVASTErrorSupportedMediaFileNotFound = 403,

    /// Problem displaying MediaFile. Media player found a MediaFile with supported type but couldn’t
    /// display it. MediaFile may include: unsupported codecs, different MIME type than MediaFile@type,
    /// unsupported delivery method, etc.
    //MPVASTErrorProblemDisplayingMediaFile = 405,

    /// Mezzanine was required but not provided. Ad not served.
    //MPVASTErrorAdNotServedSinceMezzanineWasRequiredButNotProvided = 406,

    /// Mezzanine is in the process of being downloaded for the first time. Download may take several
    /// hours. Ad will not be served until mezzanine is downloaded and transcoded.
    MPVASTErrorMezzanineIsBeingProccessed = 407,

    /// Conditional ad rejected. (deprecated along with conditionalAd)
    //MPVASTErrorConditionalAdRejected = 408,

    /// Interactive unit in the InteractiveCreativeFile node was not executed.
    //MPVASTErrorInteractiveUnitInInteractiveCreativeFileNodeWasNotExecuted = 409,

    /// Verification unit in the Verification node was not executed.
    //MPVASTErrorVerificationUnitInTheVerificationNodeWasNotExecuted = 410,

    /// Mezzanine was provided as required, but file did not meet required specification. Ad not served.
    //MPVASTErrorMezzanineWasProvidedAsRequiredButAdFileIsInvalid = 411,

    /// General NonLinearAds error.
    //MPVASTErrorGeneralNonLinearAdsError = 500,

    /// Unable to display NonLinearAd because creative dimensions do not align with creative display
    /// area (i.e. creative dimension too large).
    //MPVASTErrorNonLinearAdDimensionAlignmentError = 501,

    /// Unable to fetch NonLinearAds/NonLinear resource.
    //MPVASTErrorUnableToFetchNonLinearResource = 502,

    /// Couldn’t find NonLinear resource with supported type.
    //MPVASTErrorCannotFindNonLinearResourceWithSupportedTypes = 503,

    /// General CompanionAds error.
    MPVASTErrorGeneralCompanionAdsError = 600,

    /// Unable to display Companion because creative dimensions do not fit within Companion display
    /// area (i.e., no available space).
    //MPVASTErrorCompanionAdDimensionAlignmentError = 601,

    /// Unable to display required Companion.
    //MPVASTErrorUnableToDisplayRequiredCompanionAd = 602,

    /// Unable to fetch CompanionAds/Companion resource.
    //MPVASTErrorUnableToFetchCompanionResource = 603,

    /// Couldn’t find Companion resource with supported type.
    //MPVASTErrorCannotFindCompanionAdResourceWithSupportedTypes = 604,

    /// Undefined Error. Use this if the error cannot be described by any other error code.
    MPVASTErrorUndefined = 900,

    /// General VPAID error.
    //MPVASTErrorGeneralVPAIDError = 901,

    /// General InteractiveCreativeFile error code.
    //MPVASTErrorGeneralInteractiveCreativeFileError = 902
};

NS_ASSUME_NONNULL_END
