//
//  MRAdFullScreenViewController.h
//  MRAdMan
//
//  Created by Пучка Илья on 28.01.14.
//  Copyright (c) 2014 MailRu. All rights reserved.
//

#import <UIKit/UIKit.h>

@protocol MRAdManFullScreenViewControllerDelegate;

/**
 *  Контроллер для отображения развёрнутой на весь экран рекламы, или всплывающего окна с браузером (по клику на баннер)
 */
@interface MRAdManFullScreenViewController : UIViewController

/**
 *  Делегат контроллера
 */
@property (nonatomic, weak) id<MRAdManFullScreenViewControllerDelegate> delegate;

/**
 *  Navigation bar контроллера. 
 *  @discussion Eсли контроллер отображает развёрнутую на весь экран рекламу, то заголовок этого компонента выставляется как [MRAdManView fullScreenTitle], а сам компонент ссылается на [MRAdManView navigationBar].
 *  Eсли контроллер отображает окно с браузером (по клику на баннер), то заголовок этого компонента выставляется как название нажатого баннера.
 */
@property (nonatomic, strong) UINavigationBar *navigationBar;

/**
 *  Метод для установки заголовка кнопки закрытия контроллера
 *
 *  @param closeButtonTitle заголовок кнопки закрытия контроллера.
 *  @discussion по нажатию на эту кнопку будет вызван метод делегата контроллера [MRAdManFullScreenViewControllerDelegate fullScreenViewControllerDidFinish:];
 *  По умолчания заголовок кнопки NSLocalizedString(@"Close", @"Close")
 */
- (void)setCloseButtonTitle:(NSString *)closeButtonTitle;

- (void)setWebViewSize:(CGSize )webViewSize;

@end

/**
 *  Протокол делегата MRAdManFullScreenViewController
 */
@protocol MRAdManFullScreenViewControllerDelegate <NSObject>

/**
 *  Вызывается по нажатию кнопки закрытия в контроллере
 *
 *  @param viewController контроллер, вызвавший событие
 */
- (void)fullScreenViewControllerDidFinish:(MRAdManFullScreenViewController *)viewController;

@end
