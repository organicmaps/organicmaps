@objc(MWMUGCRatingValueType)
class UGCRatingValueType: NSObject {
  let value: String
  let type: MWMRatingSummaryViewValueType

  @objc init(value: String, type: MWMRatingSummaryViewValueType) {
    self.value = value
    self.type = type
    super.init()
  }
}

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
