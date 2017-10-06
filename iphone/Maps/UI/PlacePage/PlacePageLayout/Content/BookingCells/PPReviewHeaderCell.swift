@objc(MWMPPReviewHeaderCell)
final class PPReviewHeaderCell: UITableViewCell {
  @IBOutlet private var rating: UILabel!
  @IBOutlet private var count: UILabel!

  @objc func config(rating: UGCRatingValueType, numberOfReviews: Int) {
    self.rating.text = rating.value
    count.text = String(format: L("booking_based_on_reviews"), numberOfReviews)
  }
}
