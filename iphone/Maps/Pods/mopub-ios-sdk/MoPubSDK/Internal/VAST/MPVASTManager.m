//
//  MPVASTManager.m
//
//  Copyright 2018-2020 Twitter, Inc.
//  Licensed under the MoPub SDK License Agreement
//  http://www.mopub.com/legal/sdk-license-agreement/
//

#import "MPVASTManager.h"
#import "MPVASTAd.h"
#import "MPVASTError.h"
#import "MPVASTWrapper.h"
#import "MPXMLParser.h"
#import "MPHTTPNetworkSession.h"
#import "MPURLRequest.h"

@interface MPVASTWrapper (MPVASTManager)

@property (nonatomic, readwrite) MPVASTResponse *wrappedVASTResponse;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

static const NSInteger kMaximumWrapperDepth = 10;
static NSString * const kMPVASTManagerErrorDomain = @"com.mopub.MPVASTManager";

@implementation MPVASTManager

+ (void)fetchVASTWithData:(NSData *)data completion:(void (^)(MPVASTResponse *, NSError *))completion
{
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [self parseVASTResponseFromData:data depth:0 completion:^(MPVASTResponse *response, NSError *error) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (error) {
                    completion(nil, error);
                } else {
                    completion(response, nil);
                }
            });
        }];
    });
}

+ (void)parseVASTResponseFromData:(NSData *)data depth:(NSInteger)depth completion:(void (^)(MPVASTResponse *response, NSError *error))completion
{
    if (depth >= kMaximumWrapperDepth) {
        completion(nil, [NSError errorWithDomain:kMPVASTManagerErrorDomain code:MPVASTErrorExceededMaximumWrapperDepth userInfo:nil]);
        return;
    }

    NSError *XMLParserError = nil;
    MPXMLParser *parser = [[MPXMLParser alloc] init];
    NSDictionary *dictionary = [parser dictionaryWithData:data error:&XMLParserError];
    if (XMLParserError) {
        completion(nil, [NSError errorWithDomain:kMPVASTManagerErrorDomain code:MPVASTErrorXMLParseFailure userInfo:nil]);
        return;
    }

    MPVASTResponse *VASTResponse = [[MPVASTResponse alloc] initWithDictionary:dictionary];
    NSArray *wrappers = [self wrappersForVASTResponse:VASTResponse];
    if ([wrappers count] == 0) {
        if ([self VASTResponseContainsAtLeastOneAd:VASTResponse]) {
            completion(VASTResponse, nil);
            return;
        } else {
            completion(nil, [NSError errorWithDomain:kMPVASTManagerErrorDomain
                                                code:MPVASTErrorFailedToDisplayAdFromInlineResponse
                                            userInfo:nil]);
            return;
        }
    }

    __weak __typeof__(self) weakSelf = self;
    __block NSInteger wrappersFetched = 0;
    for (MPVASTWrapper *wrapper in wrappers) {
        [MPHTTPNetworkSession startTaskWithHttpRequest:[MPURLRequest requestWithURL:wrapper.VASTAdTagURI] responseHandler:^(NSData * _Nonnull data, NSHTTPURLResponse * _Nonnull response) {
            // Dispatch the VAST XML parsing.
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                __typeof__(self) strongSelf = weakSelf;
                [strongSelf parseVASTResponseFromData:data depth:depth + 1 completion:^(MPVASTResponse *response, NSError *error) {
                    if (error) {
                        completion(nil, error);
                        return;
                    }

                    wrapper.wrappedVASTResponse = response;
                    wrappersFetched++;

                    // Once we've fetched all wrappers within the VAST
                    // response, we can call the top-level completion
                    // handler.
                    if (wrappersFetched == [wrappers count]) {
                        if ([self VASTResponseContainsAtLeastOneAd:VASTResponse]) {
                            completion(VASTResponse, nil);
                            return;
                        } else {
                            completion(nil, [NSError errorWithDomain:kMPVASTManagerErrorDomain
                                                                code:MPVASTErrorNoVASTResponseAfterOneOrMoreWrappers
                                                            userInfo:nil]);
                            return;
                        }
                    }
                }];

            });
        } errorHandler:^(NSError * _Nonnull error) {
            wrapper.wrappedVASTResponse = nil;
        }];
    }
}

+ (NSArray *)wrappersForVASTResponse:(MPVASTResponse *)response
{
    NSMutableArray *wrappers = [NSMutableArray array];
    for (MPVASTAd *ad in response.ads) {
        if (ad.wrapper) {
            [wrappers addObject:ad.wrapper];
        }
    }
    return wrappers;
}

+ (BOOL)VASTResponseContainsAtLeastOneAd:(MPVASTResponse *)response
{
    for (MPVASTAd *ad in response.ads) {
        if (ad.inlineAd || ad.wrapper.wrappedVASTResponse) {
            return YES;
        }
    }
    return NO;
}

@end

