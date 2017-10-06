import UIKit

struct RatingSummaryViewSettings {
  enum Default {
    static let backgroundOpacity: CGFloat = 0.16
    static let colors: [MWMRatingSummaryViewValueType: UIColor] = [
      .noValue: UIColor.lightGray,
      .horrible: UIColor.red,
      .bad: UIColor.orange,
      .normal: UIColor.yellow,
      .good: UIColor.green,
      .excellent: UIColor.blue,
    ]
    static let images: [MWMRatingSummaryViewValueType: UIImage] = [:]
    static let textFont = UIFont.preferredFont(forTextStyle: UIFontTextStyle.footnote)
    static let textSize = textFont.pointSize
    static let value = "2.2"
    static let topOffset: CGFloat = 8
    static let bottomOffset: CGFloat = 8
    static let leadingImageOffset: CGFloat = 12
    static let margin: CGFloat = 8
    static let trailingTextOffset: CGFloat = 8
    static let noValueText = "—"
  }

  init() {}

  var backgroundOpacity = Default.backgroundOpacity
  var colors = Default.colors
  var images = Default.images
  var textFont = Default.textFont
  var textSize = Default.textSize
  var value = Default.value
  var topOffset = Default.topOffset
  var bottomOffset = Default.bottomOffset
  var leadingImageOffset = Default.leadingImageOffset
  var margin = Default.margin
  var trailingTextOffset = Default.trailingTextOffset
  var noValueText = Default.noValueText
}
