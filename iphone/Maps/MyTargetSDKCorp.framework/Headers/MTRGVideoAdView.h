//
//  MTRGVideoAdView.h
//  myTargetSDKCorp 4.2.6
//
//  Created by Anton Bulankin on 17.06.15.
//  Copyright (c) 2015 Mail.ru. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDKCorp/MTRGCustomParams.h>

//Доступные секции видео-рекламы
extern NSString *const kMTRGAdSectionTypeInstreamadsPreroll;
extern NSString *const kMTRGAdSectionTypeInstreamadsPostroll;
extern NSString *const kMTRGAdSectionTypeInstreamadsPauseroll;
extern NSString *const kMTRGAdSectionTypeInstreamadsMidroll;

@interface MTRGVideoBannerInfo : NSObject

//Длительность ролика в секундах
@property (nonatomic) NSTimeInterval duration;
//Можно ли закрывать
@property (nonatomic) BOOL allowClose;
//Через какое время можно закрывать
@property (nonatomic) NSTimeInterval allowCloseDelay;
//Размер баннера
@property (nonatomic) CGSize size;
//Текст кнопки
@property (nonatomic) NSString * ctaText;

@end



@class MTRGVideoAdView;

@protocol MTRGVideoAdViewDelegate <NSObject>
-(void)onLoadWithVideoAdView:(MTRGVideoAdView *)videoAdView;
-(void)onNoAdWithReason:(NSString*)reason videoAdView:(MTRGVideoAdView *)videoAdView;

@optional

-(void)onClickWithVideoAdView:(MTRGVideoAdView *)videoAdView;

//В секундах
-(void) onTimeLeftChanged:(NSTimeInterval)timeLeft duration:(NSTimeInterval)duration videoAdView:(MTRGVideoAdView*)videoAdView;
//Начался показ баннера в секции
-(void) onBannerStartWithInfo:(MTRGVideoBannerInfo*)info videoAdView:(MTRGVideoAdView*)videoAdView;
//Завершился показ баннера в секции
-(void) onBannerCompleteWithInfo:(MTRGVideoBannerInfo*)info videoAdView:(MTRGVideoAdView*)videoAdView status:(NSString*)status;
//Завершился показ всех баннеров в секции
-(void) onCompleteWithSection:(NSString*)section videoAdView:(MTRGVideoAdView*)videoAdView status:(NSString*)status;

//Воспроизведение видео приостановлено.
-(void) onBannerSuspenseWithInfo:(MTRGVideoBannerInfo*)info videoAdView:(MTRGVideoAdView*)videoAdView;
//Возобновление
-(void) onBannerResumptionWithInfo:(MTRGVideoBannerInfo*)info videoAdView:(MTRGVideoAdView*)videoAdView;
//Изменение состояния air-play
-(void) onAirPlayVideoActiveChanged:(BOOL)airPlayVideoActive videoAdView:(MTRGVideoAdView*)videoAdView;

@end


@interface MTRGVideoAdView : UIView

-(instancetype) initWithSlotId:(NSString*)slotId;

//Загрузить рекламу
-(void) load;
//Если флаг установлен в YES, ссылки и app-store будут открываться внутри приложения
@property (nonatomic) BOOL handleLinksInApp;
//Дополнительный параметры настройки запроса
@property (nonatomic, strong, readonly) MTRGCustomParams * customParams;
//Делегат
@property (weak, nonatomic) id<MTRGVideoAdViewDelegate> delegate;


-(void) startWithSection:(NSString*)section;
-(void) pause;
-(void) resume;
-(void) stop;
-(void) setFullscreen:(BOOL)isFullscreen;
-(void) setVideoQuality:(NSUInteger) quality;
-(void) setVideoPosition:(NSTimeInterval)time duration:(NSTimeInterval)duration;
-(void) closedByUser;
-(void) skipBanner;

-(void) handleClick;

@property (nonatomic, weak) UIViewController * viewController;

@end
