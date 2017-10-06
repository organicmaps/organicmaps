@objc(MWMUGCYourReview)
final class UGCYourReview: NSObject, MWMReviewProtocol {
  let date: Date
  let text: String
  let ratings: [UGCRatingStars]

  @objc init(date: Date, text: String, ratings: [UGCRatingStars]) {
    self.date = date
    self.text = text
    self.ratings = ratings
  }
}
