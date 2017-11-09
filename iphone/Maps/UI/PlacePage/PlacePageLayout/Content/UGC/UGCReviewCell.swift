@objc(MWMUGCReviewCell)
final class UGCReviewCell: MWMTableViewCell {
  @IBOutlet private weak var titleLabel: UILabel! {
    didSet {
      titleLabel.font = UIFont.bold14()
      titleLabel.textColor = UIColor.blackPrimaryText()
    }
  }

  @IBOutlet private weak var dateLabel: UILabel! {
    didSet {
      dateLabel.font = UIFont.regular12()
      dateLabel.textColor = UIColor.blackSecondaryText()
    }
  }

  @IBOutlet private weak var ratingView: RatingSummaryView! {
    didSet {
      ratingView.horribleColor = UIColor.ratingRed()
      ratingView.badColor = UIColor.ratingOrange()
      ratingView.normalColor = UIColor.ratingYellow()
      ratingView.goodColor = UIColor.ratingLightGreen()
      ratingView.excellentColor = UIColor.ratingGreen()
      ratingView.textFont = UIFont.bold16()
      ratingView.textSize = 16
    }
  }

  @IBOutlet private weak var reviewLabel: ExpandableTextView! {
    didSet {
      reviewLabel.textFont = UIFont.regular14()
      reviewLabel.textColor = UIColor.blackPrimaryText()
      reviewLabel.expandText = L("placepage_more_button")
      reviewLabel.expandTextColor = UIColor.linkBlue()
    }
  }

  @objc func config(review: UGCReview, onUpdate: @escaping () -> Void) {
    titleLabel.text = review.title
    dateLabel.text = review.date
    reviewLabel.text = review.text
    reviewLabel.onUpdate = onUpdate
    ratingView.value = review.rating.value
    ratingView.type = review.rating.type
    isSeparatorHidden = true
  }
}
