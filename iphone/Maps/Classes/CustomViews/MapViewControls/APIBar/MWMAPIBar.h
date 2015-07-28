//
//  MWMAPIBar.h
//  Maps
//
//  Created by Ilya Grechuhin on 27.07.15.
//  Copyright (c) 2015 MapsWithMe. All rights reserved.
//

@class SearchView;

typedef NS_ENUM(NSUInteger, MWMAPIBarState)
{
  MWMAPIBarStateHidden,
  MWMAPIBarStateVisible
};

@protocol MWMAPIBarProtocol <NSObject>

@property (nonnull, nonatomic) SearchView * searchView;

- (void)apiBarDidEnterState:(MWMAPIBarState)state;

@end

@interface MWMAPIBar : NSObject

@property (nonatomic, readonly) MWMAPIBarState state;
@property (nonatomic, readonly) CGRect frame;

- (nonnull instancetype)init __attribute__((unavailable("init is not available")));
- (nonnull instancetype)initWithDelegate:(nonnull UIViewController<MWMAPIBarProtocol> *)delegate;

- (void)show;

@end
