@objc(MWMUGCReviewModel)
final class UGCReviewModel: NSObject {
  let reviewValue: MWMRatingSummaryViewValueType

  let ratings: [UGCRatingStars]

  let canAddTextToReview: Bool

  let title: String
  let text: String

  @objc init(reviewValue: MWMRatingSummaryViewValueType, ratings: [UGCRatingStars], canAddTextToReview: Bool, title: String, text: String) {
    self.reviewValue = reviewValue
    self.ratings = ratings
    self.canAddTextToReview = canAddTextToReview
    self.title = title
    self.text = text
  }
}
