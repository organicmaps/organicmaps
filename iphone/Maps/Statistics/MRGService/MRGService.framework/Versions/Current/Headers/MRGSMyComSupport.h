//
//  MyComSupport.h
//  MRGServiceFramework
//
//  Created by Anton Grachev on 05.02.14.
//  Copyright (c) 2014 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>

#if (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
#import <UIKit/UIKit.h>
#endif

@protocol MRGSMyComSupportDelegate;

/** Класс MyComSupport.
 *  Предназначен для отображения окна службы поддержки My.Com в проекте.
 */
@interface MRGSMyComSupport : NSObject

/** Секретный ключ проекта для My.Com Support. */
@property (nonatomic, copy) NSString *secret;

/** Объект делегата. */
@property (nonatomic, weak) id<MRGSMyComSupportDelegate> delegate;

/** Метод для объекта-синглтона. */
+ (instancetype)sharedInstance;

#if (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
/** Метод для открытия окна службы поддержки My.Com.
 *  @param superview View, на котором будет отображено окно службы поддержки.
 */
- (void)showSupportViewOnSuperview:(UIView*)superview;
#endif

/** Метод для закрытия окна службы поддержки My.Com. */
- (void)closeSupportView;

/** Метод для проверки наличия ответов на запросы в службу поддержки. */
- (void)checkTickets;

@end

/** Протокол MyComSupportDelegate. */
@protocol MRGSMyComSupportDelegate<NSObject>

@optional
/** Метод вызывается при закрытии окна службы поддержки MyCom.
 */
- (void)myComSupportViewDidClosed;

/** Метод вызывается при возникновении ошибки в процессе загрузки страницы службы поддержки My.Com.
 *  @param error Описание ошибки.
 */
- (void)myComSupportDidReceiveError:(NSError*)error;

/** Метод вызывается в ответ на вызов '- (void)checkTickets' и при наличие ответов от службы поддержки. */
- (void)myComSupportDidReceiveNotificationsForTickets;

/** Метод вызывается в случае возникновения ошибки при запросе ответов от службы поддержки.
 *  @param error Описание ошибки.
 */
- (void)myComSupportCheckTicketsFailWithError:(NSError*)error;

@end