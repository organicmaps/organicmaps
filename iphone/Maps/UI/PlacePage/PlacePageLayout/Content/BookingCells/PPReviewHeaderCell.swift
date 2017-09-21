@objc(MWMPPReviewHeaderCell)
final class PPReviewHeaderCell: UITableViewCell {
  @IBOutlet private var rating: UILabel!
  @IBOutlet private var count: UILabel!

  @objc func config(with rate: String, numberOfReviews: Int) {
    rating.text = rate
    count.text = String(format: L("booking_based_on_reviews"), numberOfReviews)
  }
}
