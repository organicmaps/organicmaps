//
//  MTRGImageData.h
//  myTargetSDKCorp 4.2.6
//
//  Created by Anton Bulankin on 17.11.14.
//  Copyright (c) 2014 Mail.ru Group. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>


@interface MTRGImageData : NSObject

//Размер изображения, полученный с сервера
@property (nonatomic, readwrite) CGSize size;
//Загруженное изображение(будет доступно, если загрузка разрешена в параметрах)
@property (nonatomic, strong, readwrite) UIImage * image;
//URL - адрес изображения, на тот случай если загрузка осуществляется не средствами библиотеки, а вручную
@property (nonatomic, strong, readwrite) NSString * url;

//В случае если изображение не загружено, можно загрузить его этим методом
//(в момент загрузки в указанный ImageView будет установлена картинка)
-(void) loadImageToView:(UIImageView*)imageView;

@end
