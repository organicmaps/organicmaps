//
//  MPAdPlacerInvocation.h
//  MoPubSDK
//
//  Copyright (c) 2014 MoPub. All rights reserved.
//

#import <Foundation/Foundation.h>

@class MPStreamAdPlacer;

/**
 * A convenience class that handles a lot of the common logic when implementing a wrapper for the delegate / data source of a UI collection object
 * (e.g. UITableView or UICollectionView).
 *
 * When implementing wrapper object methods, you will often have to take the following into consideration:
 *
 *  - Handling cells that contain ads differently from regular content cells.  For example, you may want to disable moving an ad cell while allowing
 *  regular content cells to move.
 *
 *  - Calling through to the original object when handling regular content cells.  The original delegate / data source only knows about regular content
 *  cells.  Thus, you need to translate the given index path, that indexes ads and content, to an original index path that only indexes content.
 *  You can use the original translated index path to have the original delegate / data source process logic on the correct cell.
 *
 *  - Providing the default behavior when the original object doesn't respond to the specific method and an ad doesn't occupy the given index path.
 *
 * This class takes care of all the work in two steps.  Set up and invoke an NSInvocation object by calling invokeForTarget:.  If you wish to return
 * a value based on the invocation, you may pass the returned invocation to one of the result methods (e.g. boolResultForInvocation:defaultValue:) and
 * provide a defaultValue that will be returned if an ad occupies the cell or the original delegate / data source doesn't respond to the selector.
 * If an ad is not at the given index path and the original delegate / data source responds to the selector, invokeForTarget: will translate the index
 * path for you and pass it to the original delegate / data source.  The result method will then return the result from the invocation.
 */

@interface MPAdPlacerInvocation : NSObject

/**
 * Creates an NSInvocation object with the given parameters and invokes the object.
 * This will return nil if there is an ad at the index path or the target doesn't respond to the selector.
 *
 * @param target The object's original data source or delegate.
 * @param with2ArgSelector The method we want to execute on the target if an ad doesn't exist.
 * @param firstArg The first argument to the selector.
 * @param secondArg The second argument to the selector.
 * @param streamAdPlacer The MPStreamAdPlacer backing your UI collection that can translate index paths to their originals.
 *
 * @return The invocation with all the parameters passed into the method.
 */
+ (NSInvocation *)invokeForTarget:(id)target
                 with2ArgSelector:(SEL)selector
                         firstArg:(id)arg1
                        secondArg:(NSIndexPath *)indexPath
                   streamAdPlacer:(MPStreamAdPlacer *)streamAdPlacer;

/**
 * Creates an NSInvocation object with the given parameters and invokes the object.
 * This will return nil if there is an ad at the index path or the target doesn't respond to the selector.
 *
 * @param target The object's original data source or delegate.
 * @param with3ArgSelector The method we want to execute on the target if an ad doesn't exist.
 * @param firstArg The first argument to the selector.
 * @param secondArg The second argument to the selector.
 * @param thirdArg The third argument to the selector.
 * @param streamAdPlacer The MPStreamAdPlacer backing your UI collection that can translate index paths to their originals.
 *
 * @return The invocation with all the parameters passed into the method.
 */
+ (NSInvocation *)invokeForTarget:(id)target
                 with3ArgSelector:(SEL)selector
                         firstArg:(id)arg1
                        secondArg:(id)arg2
                         thirdArg:(NSIndexPath *)indexPath
                   streamAdPlacer:(MPStreamAdPlacer *)streamAdPlacer;

/**
 * Creates an NSInvocation object with the given parameters and invokes the object.
 * This will return nil if there is an ad at the index path or the target doesn't respond to the selector.
 *
 * @param target The object's original data source or delegate.
 * @param with3ArgSelector The method we want to execute on the target if an ad doesn't exist.
 * @param firstArg The first argument to the selector.
 * @param secondArg The second argument to the selector.
 * @param thirdArg The third argument to the selector.
 * @param streamAdPlacer The MPStreamAdPlacer backing your UI collection that can translate index paths to their originals.
 *
 * @return The invocation with all the parameters passed into the method.
 */
+ (NSInvocation *)invokeForTarget:(id)target
              with3ArgIntSelector:(SEL)selector
                         firstArg:(id)arg1
                        secondArg:(NSInteger)arg2
                         thirdArg:(NSIndexPath *)indexPath
                   streamAdPlacer:(MPStreamAdPlacer *)streamAdPlacer;

/**
 * Returns the result for an invocation.  Will return defaultReturnValue if invocation is nil.
 *
 * @param invocation The invocation that was returned from invokeForTarget:.
 * @param defaultReturnValue What to return when the invocation is nil.
 *
 * @return defaultReturnValue or the invocation's return value.
 */
+ (BOOL)boolResultForInvocation:(NSInvocation *)invocation defaultValue:(BOOL)defaultReturnValue;

/**
 * Returns the result for an invocation.  Will return defaultReturnValue if invocation is nil.
 *
 * @param invocation The invocation that was returned from invokeForTarget:.
 * @param defaultReturnValue What to return when the invocation is nil.
 *
 * @return defaultReturnValue or the invocation's return value.
 */
+ (NSInteger)integerResultForInvocation:(NSInvocation *)invocation defaultValue:(NSInteger)defaultReturnValue;

/**
 * Returns the result for an invocation.  Will return defaultReturnValue if invocation is nil.
 *
 * @param invocation The invocation that was returned from invokeForTarget:.
 * @param defaultReturnValue What to return when the invocation is nil.
 *
 * @return defaultReturnValue or the invocation's return value.
 */
+ (id)resultForInvocation:(NSInvocation *)invocation defaultValue:(id)defaultReturnValue;

@end
