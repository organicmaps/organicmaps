//
//  MPLogEventRecorder.m
//  MoPubSDK

//  Copyright (c) 2015 MoPub. All rights reserved.
//

#include <stdlib.h>
#import "MPLogEventRecorder.h"
#import "MPLogEvent.h"
#import "MPLogging.h"
#import "MPLogEventCommunicator.h"
#import "MPCoreInstanceProvider.h"
#import "MPTimer.h"

void MPAddLogEvent(MPLogEvent *event)
{
    [[[MPCoreInstanceProvider sharedProvider] sharedLogEventRecorder] addEvent:event];
}

/**
 *  The max number of events allowed in the event queue.
 */
static const NSInteger QUEUE_LIMIT = 1000;

/**
 *  The max number of events sent per request.
 */
static const NSInteger EVENT_SEND_THRESHOLD = 25;

/**
 * A number between 0 and 1 that represents the ratio at which events will be added
 * to the event queue. For example, if SAMPLE_RATE is set to 0.2, then 20% of all
 * events reported to the MPLogEventRecorder will be communicated to scribe.
 */
static const double SAMPLE_RATE = 0.1;

/**
 *  The delay in seconds to wait until sending the next batch of events.
 */
static const NSTimeInterval POLL_DELAY_INTERVAL = 2 * 60;

/**
 *  The maximum size of the requestIDLoggingCache. Note: since this is an
 *  NSCache, this is not a strict limit.
 */
static const NSInteger MAX_REQUEST_ID_CACHE_SIZE = 100;

///////////////////////////////////////////////////////////////////////////

@interface MPLogEventRecorder ()<NSURLConnectionDataDelegate>

/**
 *  IMPORTANT: All access to self.events should be performed inside a block on self.dispatchQueue.
 *  This is to prevent concurrent access issues to the event array.
 */
@property (nonatomic) dispatch_queue_t dispatchQueue;
@property (nonatomic) NSMutableArray *events;
@property (nonatomic) NSCache *requestIDLoggingCache;

@property (nonatomic) MPLogEventCommunicator *communicator;
@property (nonatomic) MPTimer *sendTimer;

@end

///////////////////////////////////////////////////////////////////////////

@implementation MPLogEventRecorder

#pragma mark - Public methods

- (instancetype)init
{
    if (self = [super init]) {
        _events = [NSMutableArray array];
        _dispatchQueue = dispatch_queue_create("com.mopub.MPLogEventRecorder", NULL);
        _communicator = [[MPLogEventCommunicator alloc] init];
        _sendTimer = [MPTimer timerWithTimeInterval:POLL_DELAY_INTERVAL
                                                 target:self
                                               selector:@selector(sendEvents)
                                                repeats:YES];
        [_sendTimer scheduleNow];
        _requestIDLoggingCache = [[NSCache alloc] init];
        _requestIDLoggingCache.countLimit = MAX_REQUEST_ID_CACHE_SIZE;
    }

    return self;
}

- (void)dealloc
{
    [self.sendTimer invalidate];
}

- (void)addEvent:(MPLogEvent *)event
{
    if (event) {
        dispatch_async(self.dispatchQueue, ^{

            // We only add the event to the queue if it's been selected for sampling.
            if (![self sampleWithLogEvent:event]) {
                MPLogDebug(@"RECORDER: Skipped adding log event to the queue because it failed the sample test.");
                return;
            }

            if ([self overQueueLimit]) {
                MPLogDebug(@"RECORDER: Skipped adding log event to the queue because the event queue is over its size limit.");
                return;
            }

            [self.events addObject:event];
            MPLogDebug([NSString stringWithFormat:@"RECORDER: Event added. There are now %lu events in the queue.", (unsigned long)[self.events count]]);
        });
    }
}

#pragma mark - Private methods

- (void)sendEvents
{
    dispatch_async(self.dispatchQueue, ^{
        MPLogDebug([NSString stringWithFormat:@"RECORDER: -sendEvents dispatched with %lu events in the queue.", (unsigned long)[self.events count]]);

        if ([self.communicator isOverLimit]) {
            MPLogDebug(@"RECORDER: Skipped sending events because the communicator has too many active connections.");
            return;
        }

        if ([self.events count] == 0) {
            return;
        }

        if ([self.events count] > EVENT_SEND_THRESHOLD) {
            MPLogDebug(@"RECORDER: Enqueueing a portion of events to be scribed.");

            // If we have more events than we can send at once, then send only the first slice.
            NSRange sendRange;
            sendRange.location = 0;
            sendRange.length = EVENT_SEND_THRESHOLD;
            NSArray *eventsToSend = [self.events subarrayWithRange:sendRange];

            // Don't flush the event array in this case, because we'll have more to send in the
            // future.
            NSRange unSentRange;
            unSentRange.location = sendRange.length;
            unSentRange.length = [self.events count] - sendRange.length;
            NSArray *unSentEvents = [self.events subarrayWithRange:unSentRange];

            [self.communicator sendEvents:eventsToSend];
            self.events = [unSentEvents mutableCopy];
        } else {
            MPLogDebug(@"RECORDER: Enqueueing all events to be scribed.");
            [self.communicator sendEvents:[NSArray arrayWithArray:self.events]];
            [self.events removeAllObjects];
        }

        MPLogDebug([NSString stringWithFormat:@"RECORDER: There are now %lu events in the queue.", (unsigned long)[self.events count]]);
    });
}

/**
 *  IMPORTANT: This method should only be called inside a block on the object's dispatch queue.
 */
- (BOOL)overQueueLimit
{
    return [self.events count] >= QUEUE_LIMIT;
}

/*
 Using this sampling method will ensure sampling decision remains constant for each request id.
 */
- (BOOL)sampleWithLogEvent:(MPLogEvent *)event
{
    BOOL samplingResult;
    if (!event.requestId) {
        samplingResult = [self sample];
    } else {
        NSNumber *existingSampleAsNumber = [self.requestIDLoggingCache objectForKey:event.requestId];
        if (existingSampleAsNumber) {
            samplingResult = [existingSampleAsNumber boolValue];
        } else {
            samplingResult = [self sample];
            [self.requestIDLoggingCache setObject:[NSNumber numberWithBool:samplingResult] forKey:event.requestId];
        }
    }
    return samplingResult;
}

- (BOOL)sample
{
    NSUInteger diceRoll = arc4random_uniform(100);
    return [self shouldSampleForRate:SAMPLE_RATE diceRoll:diceRoll];
}

/**
 *  IMPORTANT: This method takes a sample rate between 0 and 1, and the diceRoll is intended to be between 0 and 100. It has been separated from the -(BOOL)sample method for easier testing.
 */
- (BOOL)shouldSampleForRate:(CGFloat)sampleRate diceRoll:(NSUInteger)diceRoll
{
    NSUInteger sample = (NSUInteger)(sampleRate*100);
    return diceRoll < sample;
}

@end
