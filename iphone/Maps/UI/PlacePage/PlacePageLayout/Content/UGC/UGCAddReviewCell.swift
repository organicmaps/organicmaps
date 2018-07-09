@objc(MWMUGCAddReviewCell)
final class UGCAddReviewCell: MWMTableViewCell {
  @IBOutlet private weak var titleLabel: UILabel! {
    didSet {
      titleLabel.text = L("placepage_rate_comment")
      titleLabel.textColor = UIColor.blackPrimaryText()
      titleLabel.font = UIFont.medium14()
    }
  }

  @IBOutlet private weak var horribleButton: UIButton! {
    didSet {
      horribleButton.setImage(#imageLiteral(resourceName: "ic_24px_rating_horrible"), for: .normal)
      horribleButton.tintColor = UIColor.ratingRed()
    }
  }

  @IBOutlet private weak var badButton: UIButton! {
    didSet {
      badButton.setImage(#imageLiteral(resourceName: "ic_24px_rating_bad"), for: .normal)
      badButton.tintColor = UIColor.ratingOrange()
    }
  }

  @IBOutlet private weak var normalButton: UIButton! {
    didSet {
      normalButton.setImage(#imageLiteral(resourceName: "ic_24px_rating_normal"), for: .normal)
      normalButton.tintColor = UIColor.ratingYellow()
    }
  }

  @IBOutlet private weak var goodButton: UIButton! {
    didSet {
      goodButton.setImage(#imageLiteral(resourceName: "ic_24px_rating_good"), for: .normal)
      goodButton.tintColor = UIColor.ratingLightGreen()
    }
  }

  @IBOutlet private weak var excellentButton: UIButton! {
    didSet {
      excellentButton.setImage(#imageLiteral(resourceName: "ic_24px_rating_excellent"), for: .normal)
      excellentButton.tintColor = UIColor.ratingGreen()
    }
  }

  @IBOutlet private weak var horribleLabel: UILabel! {
    didSet {
      horribleLabel.text = L("placepage_rate_horrible")
      horribleLabel.textColor = UIColor.blackSecondaryText()
      horribleLabel.font = UIFont.medium10()
    }
  }

  @IBOutlet private weak var badLabel: UILabel! {
    didSet {
      badLabel.text = L("placepage_rate_bad")
      badLabel.textColor = UIColor.blackSecondaryText()
      badLabel.font = UIFont.medium10()
    }
  }

  @IBOutlet private weak var normalLabel: UILabel! {
    didSet {
      normalLabel.text = L("placepage_rate_normal")
      normalLabel.textColor = UIColor.blackSecondaryText()
      normalLabel.font = UIFont.medium10()
    }
  }

  @IBOutlet private weak var goodLabel: UILabel! {
    didSet {
      goodLabel.text = L("placepage_rate_good")
      goodLabel.textColor = UIColor.blackSecondaryText()
      goodLabel.font = UIFont.medium10()
    }
  }

  @IBOutlet private weak var excellentLabel: UILabel! {
    didSet {
      excellentLabel.text = L("placepage_rate_excellent")
      excellentLabel.textColor = UIColor.blackSecondaryText()
      excellentLabel.font = UIFont.medium10()
    }
  }

  @objc var onRateTap: ((MWMRatingSummaryViewValueType) -> Void)!

  @IBAction private func rate(_ button: UIButton) {
    switch button {
    case horribleButton: onRateTap(.horrible)
    case badButton: onRateTap(.bad)
    case normalButton: onRateTap(.normal)
    case goodButton: onRateTap(.good)
    case excellentButton: onRateTap(.excellent)
    default: assert(false)
    }
  }
}
