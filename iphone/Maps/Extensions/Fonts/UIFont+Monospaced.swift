import UIKit

extension UIFont {
  /// Creates a UIFont object with monospaced numbers keeping other font descriptors like size and weight
  @objc var monospaced: UIFont {
    let attributes: [UIFontDescriptor.AttributeName: Any] = [
      .featureSettings: [
        [
          UIFontDescriptor.FeatureKey.type: kNumberSpacingType,
          UIFontDescriptor.FeatureKey.selector: kMonospacedNumbersSelector,
        ],
      ],
    ]
    let monospacedNumbersFontDescriptor = fontDescriptor.addingAttributes(attributes)
    return UIFont(descriptor: monospacedNumbersFontDescriptor, size: pointSize)
  }
}
