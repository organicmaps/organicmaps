@objc(MWMUGCReviewModel)
final class UGCReviewModel: NSObject {
  let reviewValue: UgcSummaryRatingType

  @objc let ratings: [UGCRatingStars]
  @objc var text: String

  let title: String

  @objc init(reviewValue: UgcSummaryRatingType, ratings: [UGCRatingStars], title: String, text: String) {
    self.reviewValue = reviewValue
    self.ratings = ratings
    self.title = title
    self.text = text
  }
}
