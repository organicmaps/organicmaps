final class UGCAddReviewRatingCell: MWMTableViewCell {
  @IBOutlet weak var titleLabel: UILabel!
  @IBOutlet weak var ratingView: RatingView! {
    didSet {
      ratingView.delegate = self
    }
  }

  var model: UGCRatingStars! {
    didSet {
      titleLabel.text = L(model.title)
      ratingView.value = model.value
      ratingView.starsCount = 5
    }
  }
}

extension UGCAddReviewRatingCell: RatingViewDelegate {
  func didTouchRatingView(_ view: RatingView) {
    model.value = view.value
  }

  func didFinishTouchingRatingView(_: RatingView) {}
}
