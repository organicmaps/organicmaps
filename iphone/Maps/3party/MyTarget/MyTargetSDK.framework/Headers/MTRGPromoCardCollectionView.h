//
//  MTRGPromoCardCollectionView.h
//  MyTargetSDK
//
//  Created by Andrey Seredkin on 02.11.16.
//  Copyright © 2016 Mail.ru Group. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MyTargetSDK/MTRGNativePromoCard.h>

@protocol MTRGPromoCardCollectionViewDelegate <NSObject>

- (void)onCardClick:(MTRGNativePromoCard *)card;
- (void)onCardChange:(MTRGNativePromoCard *)card;

@end

@interface MTRGPromoCardCollectionView : UICollectionView <UICollectionViewDelegate, UICollectionViewDataSource, UICollectionViewDelegateFlowLayout>

@property (nonatomic, weak) id <MTRGPromoCardCollectionViewDelegate> cardCollectionViewDelegate;
@property (nonatomic, readonly) MTRGNativePromoCard *currentPromoCard;

- (void)setCards:(NSArray<MTRGNativePromoCard *> *)cards;

@end
