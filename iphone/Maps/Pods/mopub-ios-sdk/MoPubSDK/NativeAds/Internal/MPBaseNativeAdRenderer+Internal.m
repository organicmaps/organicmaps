//
//  MPBaseNativeAdRenderer+Internal.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPBaseNativeAdRenderer+Internal.h"
#import "MPLogging.h"
#import "MPNativeAdAdapter.h"
#import "MPNativeAdConstants.h"
#import "MPNativeAdRendering.h"

@implementation MPBaseNativeAdRenderer (Internal)

@dynamic adView;
@dynamic renderingViewClass;

- (void)renderSponsoredByTextWithAdapter:(id<MPNativeAdAdapter>)adapter {
    // Fast-fail if no label is present in the view
    if (![self.adView respondsToSelector:@selector(nativeSponsoredByCompanyTextLabel)]) {
        return;
    }

    // Generate the text
    NSString * sponsoredByText = [self generateSponsoredByTextWithAdapter:adapter];

    // Set the label with the text
    self.adView.nativeSponsoredByCompanyTextLabel.text = sponsoredByText;
    self.adView.nativeSponsoredByCompanyTextLabel.hidden = (sponsoredByText == nil);
}

- (NSString *)generateSponsoredByTextWithAdapter:(id<MPNativeAdAdapter>)adapter {
    // Fast-fail if no sponsor name is present in the ad response
    NSString * sponsorName = adapter.properties[kAdSponsoredByCompanyKey];
    if (sponsorName == nil || [sponsorName isEqualToString:@""]) {
        return nil;
    }

    NSString * sponsoredByText = nil;

    // Attempt to gather text from integration
    if ([self.renderingViewClass respondsToSelector:@selector(localizedSponsoredByTextWithSponsorName:)]) {
        sponsoredByText = [self.renderingViewClass localizedSponsoredByTextWithSponsorName:sponsorName];

        // Validate publisher string

        // Log if the string does not contain the company sponsor name
        if (![sponsoredByText containsString:sponsorName]) {
            MPLogWarn(@"Native Ad \"Sponsored by\" text does not contain the sponsor name.");
        }
    }

    // If @c sponsoredByText is still @c nil or @c @"" at this point, use default MoPub string
    if (sponsoredByText.length == 0) {
        sponsoredByText = [NSString stringWithFormat:@"Sponsored by %@", sponsorName];
    }

    return sponsoredByText;
}

@end
