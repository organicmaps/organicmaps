import UIKit

final class UGCSummaryRatingStarsCell: UICollectionViewCell {
  @IBOutlet private weak var ratingView: RatingView! {
    didSet {
      ratingView.topTextFont = UIFont.regular10()
      ratingView.topTextColor = UIColor.blackSecondaryText()
      ratingView.filledColor = UIColor.ratingYellow()
      ratingView.emptyColor = UIColor.blackDividers()
      ratingView.borderWidth = 0
    }
  }

  func config(rating: UGCRatingStars) {
    ratingView.topText = L(rating.title)
    ratingView.value = rating.value
    ratingView.starsCount = Int(rating.maxValue)
  }
}
