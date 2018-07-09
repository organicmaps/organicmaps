@objc(MWMUGCYourReview)
final class UGCYourReview: NSObject, MWMReviewProtocol {
  let date: String
  let text: String
  let ratings: [UGCRatingStars]

  @objc init(date: String, text: String, ratings: [UGCRatingStars]) {
    self.date = date
    self.text = text
    self.ratings = ratings
  }
}
