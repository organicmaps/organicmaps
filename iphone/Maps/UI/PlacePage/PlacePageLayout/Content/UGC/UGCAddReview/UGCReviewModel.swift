@objc(MWMUGCReviewModel)
final class UGCReviewModel: NSObject {
  @objc let ratings: [UGCRatingStars]
  @objc var text: String

  init(ratings: [UGCRatingStars], text: String) {
    self.ratings = ratings
    self.text = text
  }
}
