@objc(MWMUGCReview)
final class UGCReview: NSObject, MWMReviewProtocol {
  let title: String
  let date: String
  let text: String
  let rating: UGCRatingValueType

  @objc init(title: String, date: String, text: String, rating: UGCRatingValueType) {
    self.title = title
    self.date = date
    self.text = text
    self.rating = rating
  }
}
