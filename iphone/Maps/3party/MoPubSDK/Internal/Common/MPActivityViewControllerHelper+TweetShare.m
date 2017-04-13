//
//  MPActivityViewControllerHelper+TweetShare.m
//  MoPubSDK
//
//  Copyright (c) 2015 MoPub. All rights reserved.
//

#import "MPActivityViewControllerHelper+TweetShare.h"
#import "MPLogging.h"
#import "NSURL+MPAdditions.h"

static NSString * const kShareTweetScreenName = @"screen_name";
static NSString * const kShareTweetId = @"tweet_id";
static NSString * const kShareTweetURLTemplate = @"https://twitter.com/%@/status/%@";
static NSString * const kShareTweetMessageTemplate = @"Check out @%@'s Tweet: %@";

/**
 * MPSharedTweet parses an NSURL and stores the specified screenName and tweetURL.
 */

@interface MPSharedTweet : NSObject

@property (nonatomic, readonly) NSString *screenName;
@property (nonatomic, readonly) NSString *tweetURL;

- (instancetype)initWithShareURL:(NSURL *)URL;

@end

@implementation MPSharedTweet

- (instancetype)initWithShareURL:(NSURL *)URL
{
    self = [super init];
    if (self) {
        NSDictionary *queryParamDict = [URL mp_queryAsDictionary];
        id screenName = [queryParamDict objectForKey:kShareTweetScreenName];
        id tweetId = [queryParamDict objectForKey:kShareTweetId];

        // Fail initialization if the provided URL is not of the correct format.
        // Both parameters are required.
        if (screenName && tweetId) {
            _screenName = screenName;
            _tweetURL = [NSString stringWithFormat:kShareTweetURLTemplate, screenName, tweetId];
        } else {
            MPLogDebug(@"MPActivityViewControllerHelper+TweetShare - \
                       unable to initWithShareURL for share URL: %@. \
                       screen_name or tweet_id missing or of the wrong \
                       format", [URL absoluteString]);
            return nil;
        }
    }
    return self;
}

@end

@implementation MPActivityViewControllerHelper (TweetShare)

- (BOOL)presentActivityViewControllerWithTweetShareURL:(NSURL *)URL
{
    MPSharedTweet *sharedTweet = [[MPSharedTweet alloc] initWithShareURL:URL];
    if (sharedTweet) {
        NSString *tweetMessage = [NSString stringWithFormat:kShareTweetMessageTemplate,
                                  sharedTweet.screenName, sharedTweet.tweetURL];
        return [self presentActivityViewControllerWithSubject:tweetMessage body:tweetMessage];
    }
    return NO;
}

@end
