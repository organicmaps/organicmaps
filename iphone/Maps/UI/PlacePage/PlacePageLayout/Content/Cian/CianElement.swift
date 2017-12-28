final class CianElement: UICollectionViewCell {
  enum State {
    case pending(onButtonAction: () -> Void)
    case offer(model: CianItemModel?, onButtonAction: (CianItemModel?) -> Void)
    case error(onButtonAction: () -> Void)
  }

  @IBOutlet private var contentViews: [UIView]!

  @IBOutlet private weak var pendingView: UIView!
  @IBOutlet private weak var offerView: UIView!
  @IBOutlet private weak var more: UIButton!
  @IBOutlet private weak var price: UILabel! {
    didSet {
      price.font = UIFont.medium14()
      price.textColor = UIColor.linkBlue()
    }
  }

  @IBOutlet private weak var descr: UILabel! {
    didSet {
      descr.font = UIFont.medium14()
      descr.textColor = UIColor.blackPrimaryText()
    }
  }

  @IBOutlet private weak var address: UILabel! {
    didSet {
      address.font = UIFont.regular12()
      address.textColor = UIColor.blackSecondaryText()
    }
  }

  @IBOutlet private weak var details: UIButton! {
    didSet {
      details.setTitleColor(UIColor.linkBlue(), for: .normal)
      details.setBackgroundColor(UIColor.blackOpaque(), for: .normal)
    }
  }

  private var isLastCell = false {
    didSet {
      more.isHidden = !isLastCell
      price.isHidden = isLastCell
      descr.isHidden = isLastCell
      address.isHidden = isLastCell
      details.isHidden = isLastCell
    }
  }

  @IBOutlet private weak var pendingSpinnerView: UIImageView! {
    didSet {
      pendingSpinnerView.tintColor = UIColor.linkBlue()
    }
  }

  @IBOutlet private weak var pendingTitleTopOffset: NSLayoutConstraint!
  @IBOutlet private weak var pendingTitle: UILabel! {
    didSet {
      pendingTitle.font = UIFont.medium14()
      pendingTitle.textColor = UIColor.blackPrimaryText()
      pendingTitle.text = L("preloader_cian_title")
    }
  }

  @IBOutlet private weak var pendingDescription: UILabel! {
    didSet {
      pendingDescription.font = UIFont.regular12()
      pendingDescription.textColor = UIColor.blackSecondaryText()
      pendingDescription.text = L("preloader_cian_message")
    }
  }

  @IBAction func onButtonAction() {
    switch state! {
    case let .pending(action): action()
    case let .offer(model, action): action(model)
    case let .error(action): action()
    }
  }

  var state: State! {
    didSet {
      setupAppearance()
      let visibleView: UIView
      let pendingSpinnerViewAlpha: CGFloat
      switch state! {
      case .pending:
        pendingSpinnerViewAlpha = 1
        visibleView = self.pendingView
      case .offer:
        pendingSpinnerViewAlpha = 1
        visibleView = self.offerView
      case .error:
        pendingSpinnerViewAlpha = 0
        visibleView = self.pendingView
      }

      animateConstraints(animations: {
        self.contentViews.forEach { $0.isHidden = false }
        switch self.state! {
        case .pending: self.configPending()
        case let .offer(model, _): self.configOffer(model: model)
        case .error: self.configError()
        }
        self.pendingSpinnerView.alpha = pendingSpinnerViewAlpha
        self.contentViews.forEach { $0.alpha = 0 }
        visibleView.alpha = 1
      }, completion: {
        self.contentViews.forEach { $0.isHidden = true }
        visibleView.isHidden = false
      })
    }
  }

  private var isSpinning = false {
    didSet {
      let animationKey = "SpinnerAnimation"
      if isSpinning {
        let animation = CABasicAnimation(keyPath: "transform.rotation.z")
        animation.fromValue = NSNumber(value: 0)
        animation.toValue = NSNumber(value: 2 * Double.pi)
        animation.duration = 0.8
        animation.repeatCount = Float.infinity
        pendingSpinnerView.layer.add(animation, forKey: animationKey)
      } else {
        pendingSpinnerView.layer.removeAnimation(forKey: animationKey)
      }
    }
  }

  private func setupAppearance() {
    backgroundColor = UIColor.white()
    layer.cornerRadius = 6
    layer.borderWidth = 1
    layer.borderColor = UIColor.blackDividers().cgColor
  }

  private func configPending() {
    isSpinning = true
    details.setTitle(L("preloader_cian_button"), for: .normal)
    pendingTitleTopOffset.priority = UILayoutPriority.defaultLow
  }

  private func configError() {
    isSpinning = false
    details.setTitle(L("preloader_cian_button"), for: .normal)
    pendingTitleTopOffset.priority = UILayoutPriority.defaultHigh
  }

  private func configOffer(model: CianItemModel?) {
    isSpinning = false
    if let model = model {
      isLastCell = false

      let priceFormatter = NumberFormatter()
      priceFormatter.usesGroupingSeparator = true
      if let priceString = priceFormatter.string(from: NSNumber(value: model.priceRur)) {
        price.text = "\(priceString) \(L("rub_month"))"
      } else {
        price.isHidden = true
      }

      let descrFormat = L("room").replacingOccurrences(of: "%s", with: "%@")
      descr.text = String(format: descrFormat, arguments: ["\(model.roomsCount)"])

      address.text = model.address

      details.setTitle(L("details"), for: .normal)
    } else {
      isLastCell = true

      more.setBackgroundImage(UIColor.isNightMode() ? #imageLiteral(resourceName: "btn_float_more_dark") : #imageLiteral(resourceName: "btn_float_more_light"), for: .normal)

      backgroundColor = UIColor.clear
      layer.borderColor = UIColor.clear.cgColor
    }
  }
}
