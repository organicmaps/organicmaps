NS_ASSUME_NONNULL_BEGIN

@interface MWMDiscoveryGuideViewModel : NSObject

@property(nonatomic, readonly) NSString *title;
@property(nonatomic, readonly) NSString *subtitle;
@property(nonatomic, nullable, readonly) NSString *label;
@property(nonatomic, nullable, readonly) NSString *labelHexColor;
@property(nonatomic, nullable, readonly) NSString *imagePath;

- (instancetype)initWithTitle:(NSString *)title
                     subtitle:(NSString *)subtitle
                        label:(nullable NSString *)label
                labelHexColor:(nullable NSString *)labelHexColor
                     imageURL:(nullable NSString *) imagePath;

@end

NS_ASSUME_NONNULL_END
