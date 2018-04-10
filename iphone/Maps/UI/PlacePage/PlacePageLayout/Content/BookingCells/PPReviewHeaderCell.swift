@objc(MWMPPReviewHeaderCell)
final class PPReviewHeaderCell: UITableViewCell {
  @IBOutlet private var rating: UILabel!
  @IBOutlet private var count: UILabel!

  @objc func config(rating: UGCRatingValueType, numberOfReviews: Int) {
    self.rating.text = rating.value
    count.text = String(coreFormat: L("placepage_summary_rating_description"), arguments: [numberOfReviews])
  }
}
