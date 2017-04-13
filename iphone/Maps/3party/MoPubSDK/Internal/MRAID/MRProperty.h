//
//  MRProperty.h
//  MoPub
//
//  Copyright (c) 2011 MoPub, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "MRConstants.h"

@interface MRProperty : NSObject

- (NSString *)description;
- (NSString *)jsonString;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRHostSDKVersionProperty : MRProperty

@property (nonatomic, copy) NSString *version;

+ (MRHostSDKVersionProperty *)defaultProperty;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRPlacementTypeProperty : MRProperty {
    MRAdViewPlacementType _placementType;
}

@property (nonatomic, assign) MRAdViewPlacementType placementType;

+ (MRPlacementTypeProperty *)propertyWithType:(MRAdViewPlacementType)type;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRStateProperty : MRProperty {
    MRAdViewState _state;
}

@property (nonatomic, assign) MRAdViewState state;

+ (MRStateProperty *)propertyWithState:(MRAdViewState)state;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRScreenSizeProperty : MRProperty {
    CGSize _screenSize;
}

@property (nonatomic, assign) CGSize screenSize;

+ (MRScreenSizeProperty *)propertyWithSize:(CGSize)size;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRSupportsProperty : MRProperty

@property (nonatomic, assign) BOOL supportsSms;
@property (nonatomic, assign) BOOL supportsTel;
@property (nonatomic, assign) BOOL supportsCalendar;
@property (nonatomic, assign) BOOL supportsStorePicture;
@property (nonatomic, assign) BOOL supportsInlineVideo;

+ (NSDictionary *)supportedFeatures;
+ (MRSupportsProperty *)defaultProperty;
+ (MRSupportsProperty *)propertyWithSupportedFeaturesDictionary:(NSDictionary *)dictionary;

@end

////////////////////////////////////////////////////////////////////////////////////////////////////

@interface MRViewableProperty : MRProperty {
    BOOL _isViewable;
}

@property (nonatomic, assign) BOOL isViewable;

+ (MRViewableProperty *)propertyWithViewable:(BOOL)viewable;

@end
