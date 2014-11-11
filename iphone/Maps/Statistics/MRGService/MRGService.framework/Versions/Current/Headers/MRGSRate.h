//  $Id: MRGSRate.h 5723 2014-10-23 09:03:06Z a.grachev $
//
//  MRGSRate.h
//  MRGServiceFramework
//
//  Created by AKEB on 22.04.13.
//  Copyright (c) 2013 Mail.Ru Games. All rights reserved.
//

#ifndef MRGServiceFramework_MRGSRate_
#define MRGServiceFramework_MRGSRate_

#import <Foundation/Foundation.h>
#if (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

@protocol MRGSRateDelegate;

/** Класс MRGSRate для отображения окна отзывов и Feedback */
@interface MRGSRate : NSObject

/** Делегат класса.
 */
@property (nonatomic, weak) id<MRGSRateDelegate> delegate;

#if (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
/** UIView на котором будет отображаться окно для Feedback */
@property (nonatomic, strong) UIView* rootView;
#else
/** NSWindow на котором будет отображаться окно для Feedback и NSAlert */
@property (nonatomic, strong) NSWindow* rootWindow;
#endif

/** Заголовок окна Алерта */
@property (nonatomic, copy) NSString* alertTitle;

/** Текст кнопки Rate Now */
@property (nonatomic, copy) NSString* alertRateButton;

/** Текст кнопки Send Feedback */
@property (nonatomic, copy) NSString* alertFeedbackButton;

/** Текс кнопки Not now */
@property (nonatomic, copy) NSString* alertCancelButton;

/** Ссылка на приложение в AppStore */
@property (nonatomic, copy) NSString* rateURL;

/** Текст для окна FeedBack */
@property (nonatomic, copy) NSString* FeedBackTitle;
/** Заголовок поля Email */
@property (nonatomic, copy) NSString* FeedBackEmailTitle;
/** Заголовок поля Subject */
@property (nonatomic, copy) NSString* FeedBackSubjectTitle;
/** Заголовок поля Body */
@property (nonatomic, copy) NSString* FeedBackBodyTitle;
/** Текст кнопки отправить */
@property (nonatomic, copy) NSString* FeedBackSendButton;
/** Текст заголовка окна с ошибкой */
@property (nonatomic, copy) NSString* FeedBackErrorTitle;
/** Текст ошибки о неправильном формате email адреса */
@property (nonatomic, copy) NSString* FeedBackEmailFormatErrorMessage;
/** Текст кнопки на окне с ошибкой */
@property (nonatomic, copy) NSString* FeedBackErrorButton;
/** Текст сообщения об успешной отправки письма в службу поддержки */
@property (nonatomic, copy) NSString* FeedBackEmailSentMessage;

/** Использовать тему сообщения или нет */
@property (nonatomic) BOOL FeedBackSubject;

#if (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
/** Инициализация MRGSRate
	 * @param view UIView на котором будет отображаться окно для Feedback
	 * @return экземпляр класса MRGSRate
	 */
- (instancetype)initWithView:(UIView*)view NS_DESIGNATED_INITIALIZER;
#else
/** Инициализация MRGSRate
	 * @param window NSWindow на котором будет отображаться окно для Feedback и NSAlert
	 * @return экземпляр класса MRGSRate
	 */
- (id)initWithWindow:(NSWindow*)window;
#endif

/** Пользователь уже переходил в AppStore для голосования
 * @return статус перехода
 */
+ (BOOL)isRated;

/** Показать окно рейтинга */
- (void)showMessage;

/** Показать окно рейтинга 
 @param showFeedBack Yes - показывает кнопку для FeedBack
 */
- (void)showMessageWithFeedBack:(BOOL)showFeedBack;

/** Показать окно FeedBack */
- (void)showFeedback;

@end

#pragma mark -
#pragma mark Протокол
/**  @name Протокол */

/** Протокол MRGSMoreGamesDelegate. */
@protocol MRGSRateDelegate<NSObject>

@optional

/**
 *   Метод вызывается при закрытии окна
 */
- (void)feedbackViewDidClosed;

@end

#endif