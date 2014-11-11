//
//  SKStoreProductViewControllerDelegateExtended.h
//  MRAdMan
//
//  Created by Пучка Илья on 17.02.14.
//  Copyright (c) 2014 Mail.ru. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <StoreKit/SKStoreProductViewController.h>

/**
 *  Расширение стандартного протокола SKStoreProductViewControllerDelegate
 */
@protocol SKStoreProductViewControllerDelegateExtended <SKStoreProductViewControllerDelegate>

/**
 *  Метод вызывается, когда SKStoreProductViewController заканчивает загрузку данных
 *
 *  @param viewController контроллер
 *  @param success        true, если загрузка прошла успешно
 *  @param error          ошибка, возникшая при загрузке, или nil
 */
- (void)productViewControllerDidFinishLoad:(SKStoreProductViewController *)viewController
                                   success:(BOOL)success
                                     error:(NSError *)error;

@end
