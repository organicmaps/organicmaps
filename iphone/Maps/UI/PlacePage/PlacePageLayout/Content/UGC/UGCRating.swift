@objc(MWMUGCRatingStars)
final class UGCRatingStars: NSObject {
  @objc let title: String
  @objc var value: CGFloat
  let maxValue: CGFloat

  @objc init(title: String, value: CGFloat, maxValue: CGFloat) {
    self.title = title
    self.value = value
    self.maxValue = maxValue
  }
}
