@objc(MWMNavigationAddPointToastView)
final class NavigationAddPointToastView: UIVisualEffectView {
  @IBOutlet private var descriptionLabel: UILabel! {
    didSet {
      descriptionLabel.font = UIFont.regular14()
      descriptionLabel.textColor = UIColor.whitePrimaryText()
    }
  }
  @IBOutlet private var actionButton: UIButton! {
    didSet {
      actionButton.setTitle(L("button_use"), for: .normal)
      actionButton.setTitleColor(UIColor.linkBlue(), for: .normal)
      actionButton.tintColor = UIColor.linkBlue()
      actionButton.titleLabel?.font = UIFont.regular17()

      let flipTransform = CGAffineTransform(scaleX: -1, y: 1)
      actionButton.transform = flipTransform
      actionButton.titleLabel?.transform = flipTransform
      actionButton.imageView?.transform = flipTransform
    }
  }
  @IBOutlet private var actionButtonHidden: NSLayoutConstraint!

  @IBOutlet private var multilineConstraints: [NSLayoutConstraint]!

  func config(text: String, withActionButton: Bool) {
    descriptionLabel.text = text

    setNeedsLayout()
    if withActionButton {
      actionButtonHidden.priority = UILayoutPriorityDefaultHigh
      actionButton.isHidden = false
    } else {
      actionButtonHidden.priority = UILayoutPriorityDefaultLow
      actionButton.isHidden = true
    }
    UIView.animate(withDuration: kDefaultAnimationDuration) { self.layoutIfNeeded() }
  }

  private var recentSuperviewSize: CGSize? {
    didSet {
      guard recentSuperviewSize != oldValue else { return }
      DispatchQueue.main.async {
        self.descriptionLabelLines = self.descriptionLabel.numberOfVisibleLines
      }
    }
  }

  private var descriptionLabelLines = 0 {
    didSet {
      guard descriptionLabelLines != oldValue else { return }
      let priority: UILayoutPriority
      if recentSuperviewSize!.width > recentSuperviewSize!.height {
        priority = UILayoutPriorityDefaultLow
      } else {
        priority = descriptionLabelLines > 1 ? UILayoutPriorityDefaultHigh : UILayoutPriorityDefaultLow
      }
      multilineConstraints.forEach { $0.priority = priority }
      setNeedsLayout()
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    recentSuperviewSize = superview?.frame.size
  }
}
