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
      ratingView.defaultConfig()
      ratingView.textFont = UIFont.bold16()
      ratingView.textSize = 16
    }
  }

  @IBOutlet private weak var reviewLabel: ExpandableReviewView! {
    didSet {
      let settings = ExpandableReviewSettings(expandText: L("placepage_more_button"),
                                              expandTextColor: .linkBlue(),
                                              textColor: .blackPrimaryText(),
                                              textFont: .regular14())
      reviewLabel.apply(settings: settings)
    }
  }

  @objc func config(review: UGCReview, isExpanded: Bool, onUpdate: @escaping () -> Void) {
    titleLabel.text = review.title
    dateLabel.text = review.date
    reviewLabel.configure(text: review.text,
                          isExpanded: isExpanded,
                          onUpdate: onUpdate)
    ratingView.value = review.rating.value
    ratingView.type = review.rating.type
    isSeparatorHidden = true
  }
}
