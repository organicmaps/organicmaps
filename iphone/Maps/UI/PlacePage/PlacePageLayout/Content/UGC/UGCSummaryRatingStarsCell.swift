
final class UGCSummaryRatingStarsCell: UICollectionViewCell {
  @IBOutlet private weak var ratingView: RatingView!

  func config(rating: UGCRatingStars) {
    ratingView.topText = L(rating.title)
    ratingView.value = rating.value
    ratingView.starsCount = Int(rating.maxValue)
  }
}
