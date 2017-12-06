@objc(MWMViatorElement)
final class ViatorElement: UICollectionViewCell {
  @IBOutlet private weak var more: UIButton!
  @IBOutlet private weak var priceView: UIView!

  @IBOutlet private weak var image: UIImageView!
  @IBOutlet private weak var title: UILabel! {
    didSet {
      title.font = UIFont.medium14()
      title.textColor = UIColor.blackPrimaryText()
    }
  }

  @IBOutlet private weak var duration: UILabel! {
    didSet {
      duration.font = UIFont.regular12()
      duration.textColor = UIColor.blackSecondaryText()
    }
  }

  @IBOutlet private weak var price: UILabel! {
    didSet {
      price.textColor = UIColor.linkBlue()
      price.font = UIFont.medium14()
    }
  }

  @IBOutlet private weak var rating: RatingSummaryView! {
    didSet {
      rating.defaultConfig()
      rating.textFont = UIFont.bold12()
      rating.textSize = 12
    }
  }

  private var isLastCell = false {
    didSet {
      more.isHidden = !isLastCell
      image.isHidden = isLastCell
      title.isHidden = isLastCell
      duration.isHidden = isLastCell
      price.isHidden = isLastCell
      rating.isHidden = isLastCell
      priceView.isHidden = isLastCell
    }
  }

  override var isHighlighted: Bool {
    didSet {
      guard model != nil else { return }
      UIView.animate(withDuration: kDefaultAnimationDuration,
                     delay: 0,
                     options: [.allowUserInteraction, .beginFromCurrentState],
                     animations: { self.alpha = self.isHighlighted ? 0.3 : 1 },
                     completion: nil)
    }
  }

  @IBAction
  private func onMore() {
    onMoreAction?()
  }

  @objc
  var model: ViatorItemModel? {
    didSet {
      if let model = model {
        image.af_setImage(withURL: model.imageURL, imageTransition: .crossDissolve(kDefaultAnimationDuration))
        title.text = model.title
        duration.text = model.duration
        price.text = String(coreFormat: L("place_page_starting_from"), arguments: [model.price])
        rating.value = model.ratingFormatted
        rating.type = model.ratingType

        backgroundColor = UIColor.white()
        layer.cornerRadius = 6
        layer.borderWidth = 1
        layer.borderColor = UIColor.blackDividers().cgColor

        isLastCell = false
      } else {
        more.setImage(UIColor.isNightMode() ? #imageLiteral(resourceName: "btn_float_more_dark") : #imageLiteral(resourceName: "btn_float_more_light"), for: .normal)

        backgroundColor = UIColor.clear
        layer.borderColor = UIColor.clear.cgColor

        isLastCell = true
      }
    }
  }

  var onMoreAction: (() -> Void)?
}
