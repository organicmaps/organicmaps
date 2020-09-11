//
//  MPNativeAdError+VAST.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPNativeAdError+VAST.h"

MPVASTError VASTErrorCodeFromNativeAdErrorCode(MPNativeAdErrorCode nativeAdErrorCode) {
    switch (nativeAdErrorCode) {
        case MPNativeAdErrorUnknown:
            return MPVASTErrorUndefined;
        case MPNativeAdErrorHTTPError:
            return MPVASTErrorTimeoutOfMediaFileURI;
        case MPNativeAdErrorInvalidServerResponse:
            return MPVASTErrorUndefined;
        case MPNativeAdErrorNoInventory:
            return MPVASTErrorUnableToFindLinearAdOrMediaFileFromURI;
        case MPNativeAdErrorImageDownloadFailed:
            return MPVASTErrorCannotPlayMedia;
        case MPNativeAdErrorAdUnitWarmingUp:
            return MPVASTErrorMezzanineIsBeingProccessed;
        case MPNativeAdErrorVASTParsingFailed:
            return MPVASTErrorXMLParseFailure;
        case MPNativeAdErrorVideoConfigInvalid:
            return MPVASTErrorXMLParseFailure;
        case MPNativeAdErrorContentDisplayError:
            return MPVASTErrorCannotPlayMedia;
        case MPNativeAdErrorRenderError:
            return MPVASTErrorCannotPlayMedia;
    }
}
