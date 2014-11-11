//  $Id: MRGSBank.h 5660 2014-10-20 13:52:38Z a.grachev $
//
//  MRGSBank.h
//  MRGServiceFramework
//
//  Created by Yuriy Lisenkov on 19.10.12.
//  Copyright (c) 2012 Mail.Ru Games. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>
#import "MRGS.h"

@protocol MRGSBankDelegate;

/**  Класс MRGSBank. Синглтонный класс, существовать должен только 1 экземпляр класса. */
@interface MRGSBank : NSObject<SKPaymentTransactionObserver, SKProductsRequestDelegate>

/** Массив с продуктами */
@property (nonatomic, strong, readonly) NSMutableDictionary* products;
/** Делегат MRGSBankDelegate для ответов */
@property (nonatomic, weak) id<MRGSBankDelegate> delegate;

/**
 *  Экземпляр класса MRGSBank.
 *
 *  @return Возвращает экземпляр класса MRGSBank
 */
+ (instancetype)sharedInstance;

/**
 *  Делает запрос в Apple для получения информации о заведенных в iTunes Connect платежах.
 *
 *  @param productIdentifiers идентификаторы покупок, информацию о которых необходимо запросить у Apple
 */
- (void)loadProductsFromAppleServer:(NSSet*)productIdentifiers;

/**
 *  Добавляет платеж в очередь платежей.
 *
 *  @param paymentIdentifier @param paymentIdentifier идентифиактор платежа.
 */
- (void)addPayment:(NSString*)paymentIdentifier;

/**
 *  Закрывает платеж на сервере. Если игра клиент-серверная и игровую валюту выдает сервер, а не клиент, то вызывать метод не нужно!!!
 *
 *  @param transaction информация о транзакции.
 */
- (void)closePayment:(SKPaymentTransaction*)transaction;

/**
 *  Закрытие платежа на сервере
 *
 *  @param transactionId     Идентификатор транзакции
 *  @param productIdentifier Идентификатор платежа
 */
- (void)closePaymentWithTransactionId:(NSString*)transactionId andProductIdentifier:(NSString*)productIdentifier;

/**
 *  Восстанавление покупок
 */
- (void)restorePurchase;

/**
 *  Отправляет на сервер статистики информацию о проведенных платежах, которые были произведены не средствами MRGServiceFramework.
 *
 *  @param product     Информация о продукте
 *  @param transaction Информация о транзакции
 */
- (void)sendPaymentInfoForProduct:(SKProduct*)product transaction:(SKPaymentTransaction*)transaction;

#pragma mark - Deprecated methods and properties
/**
 *  Экземпляр класса MRGSBank.
 *
 *  @return Возвращает экземпляр класса MRGSBank
 *  @deprecated Используйте метод [MRGSBank sharedInstance];
 */
+ (MRGSBank*)singleton DEPRECATED_ATTRIBUTE;

/**
 *  Восстанавливает не завершенные покупки
 */
- (void)restorePurched DEPRECATED_ATTRIBUTE;

@end

/** Протокол MRGSProductsRequestDelegate. */
@protocol MRGSBankDelegate<NSObject>

@required
/**
 *  Метод протокола вызывается при завершении запроса информации о заведенных в iTunes Connect платежах
 *
 *  @param response Информация о заведенных товарах
 */
- (void)loadingPaymentsResponce:(SKProductsResponse*)response;

/**
 *  Метод протокола, вызывается в случае успешного завершения платежа
 *
 *  @param transaction Информация о завершенной транзакции
 *  @param answer      Ответ сервера
 */
- (void)paymentSuccessful:(SKPaymentTransaction*)transaction answer:(NSString*)answer;

/**
 *  Метод протокола, вызывается в случае завершения платежа с ошибкой
 *
 *  @param transaction Информация о завершенной транзакции
 *  @param error       Описание ошибки
 */
- (void)paymentFailed:(SKPaymentTransaction*)transaction error:(NSError*)error;

@end
