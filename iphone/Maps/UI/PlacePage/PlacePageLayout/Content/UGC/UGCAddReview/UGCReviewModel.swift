@objc(MWMUGCReviewModel)
final class UGCReviewModel: NSObject {
  let reviewValue: MWMRatingSummaryViewValueType

  @objc let ratings: [UGCRatingStars]
  @objc var text: String

  let title: String

  @objc init(reviewValue: MWMRatingSummaryViewValueType, ratings: [UGCRatingStars], title: String, text: String) {
    self.reviewValue = reviewValue
    self.ratings = ratings
    self.title = title
    self.text = text
  }
}
