@import UIKit;

NS_ASSUME_NONNULL_BEGIN

/// UIActivityItemSource for sharing a local file with a human-readable display name.
/// Sets NSItemProvider.suggestedName so the recipient sees the original name,
/// and provides LPLinkMetadata for the share sheet preview.
@interface MWMFileShareActivityItem : NSObject <UIActivityItemSource>

- (instancetype)initWithFileURL:(NSURL *)url displayName:(NSString *)displayName;

@end

NS_ASSUME_NONNULL_END
