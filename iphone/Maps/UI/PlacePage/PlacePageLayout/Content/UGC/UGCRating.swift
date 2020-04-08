@objc(MWMUGCRatingStars)
final class UGCRatingStars: NSObject {
  @objc let title: String
  @objc var value: CGFloat

  init(title: String, value: CGFloat) {
    self.title = title
    self.value = value
  }
}
