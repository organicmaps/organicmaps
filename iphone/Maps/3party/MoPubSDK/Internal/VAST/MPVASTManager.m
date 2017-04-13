//
//  MPVASTManager.m
//  MoPub
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPVASTManager.h"
#import "MPVASTAd.h"
#import "MPVASTWrapper.h"
#import "MPXMLParser.h"

@interface MPVASTWrapper (MPVASTManager)

@property (nonatomic, readwrite) MPVASTResponse *wrappedVASTResponse;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

static const NSInteger kMaximumWrapperDepth = 10;
static NSString * const kMPVASTManagerErrorDomain = @"com.mopub.MPVASTManager";

@implementation MPVASTManager

+ (void)fetchVASTWithURL:(NSURL *)URL completion:(void (^)(MPVASTResponse *, NSError *))completion
{
    [NSURLConnection sendAsynchronousRequest:[NSURLRequest requestWithURL:URL]
                                       queue:[NSOperationQueue mainQueue]
                           completionHandler:^(NSURLResponse *response, NSData *data, NSError *connectionError) {
                               [self fetchVASTWithData:data completion:completion];
                           }];
}

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
            completion(nil, [NSError errorWithDomain:kMPVASTManagerErrorDomain code:MPVASTErrorNoAdsFound userInfo:nil]);
            return;
        }
    }

    __block NSInteger wrappersFetched = 0;
    for (MPVASTWrapper *wrapper in wrappers) {
        [NSURLConnection sendAsynchronousRequest:[NSURLRequest requestWithURL:wrapper.VASTAdTagURI]
                                           queue:[NSOperationQueue mainQueue]
                               completionHandler:^(NSURLResponse *response, NSData *data, NSError *connectionError) {
                                   dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                                       if (connectionError) {
                                           wrapper.wrappedVASTResponse = nil;
                                       } else if (data) {
                                           [self parseVASTResponseFromData:data depth:depth + 1 completion:^(MPVASTResponse *response, NSError *error) {
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
                                                       completion(nil, [NSError errorWithDomain:kMPVASTManagerErrorDomain code:MPVASTErrorNoAdsFound userInfo:nil]);
                                                       return;
                                                   }
                                               }
                                           }];
                                       }
                                   });
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

