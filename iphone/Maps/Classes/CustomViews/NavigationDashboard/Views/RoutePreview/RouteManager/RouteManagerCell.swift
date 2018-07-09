final class RouteManagerCell: MWMTableViewCell {
  @IBOutlet private weak var typeImage: UIImageView!
  @IBOutlet private weak var titleLabel: UILabel! {
    didSet {
      titleLabel.textColor = UIColor.blackPrimaryText()
      titleLabel.font = UIFont.regular16()
    }
  }

  @IBOutlet weak var subtitleLabel: UILabel! {
    didSet {
      subtitleLabel.textColor = UIColor.blackSecondaryText()
      subtitleLabel.font = UIFont.regular14()
    }
  }

  @IBOutlet private weak var dragImage: UIImageView! {
    didSet {
      dragImage.image = #imageLiteral(resourceName: "ic_route_manager_move")
      dragImage.tintColor = UIColor.blackHintText()
    }
  }

  @IBOutlet private weak var separator1: UIView! {
    didSet {
      separator1.backgroundColor = UIColor.blackDividers()
      separator1.layer.cornerRadius = 2
    }
  }

  @IBOutlet weak var separator2: UIView! {
    didSet {
      separator2.backgroundColor = UIColor.blackDividers()
    }
  }

  private var index: Int!
  private var model: MWMRoutePoint!
  @IBOutlet var subtitleConstraints: [NSLayoutConstraint]!

  override var snapshot: UIView {
    let skipViews: [UIView] = [typeImage, separator1, separator2]
    skipViews.forEach { $0.isHidden = true }
    backgroundColor = UIColor.blackOpaque()
    let snapshot = super.snapshot
    backgroundColor = UIColor.white()
    skipViews.forEach { $0.isHidden = false }
    return snapshot
  }

  func set(model: MWMRoutePoint, atIndex index: Int) {
    self.model = model
    self.index = index

    backgroundColor = UIColor.white()
    setupTypeImage()
    setupLabels()
    setupSeparators()
  }

  private func setupTypeImage() {
    if model.isMyPosition && index == 0 {
      typeImage.image = #imageLiteral(resourceName: "ic_route_manager_my_position")
      typeImage.tintColor = UIColor.linkBlue()
    } else {
      switch model.type {
      case .start:
        typeImage.image = #imageLiteral(resourceName: "ic_route_manager_start")
        typeImage.tintColor = UIColor.linkBlue()
      case .intermediate:
        switch model.intermediateIndex {
        case 0:
          typeImage.image = #imageLiteral(resourceName: "ic_route_manager_stop_a")
          typeImage.tintColor = UIColor.primary()
        case 1:
          typeImage.image = #imageLiteral(resourceName: "ic_route_manager_stop_b")
          typeImage.tintColor = UIColor.primary()
        case 2:
          typeImage.image = #imageLiteral(resourceName: "ic_route_manager_stop_c")
          typeImage.tintColor = UIColor.primary()
        default: fatalError("Unsupported route point intermediateIndex.")
        }
      case .finish:
        typeImage.image = #imageLiteral(resourceName: "ic_route_manager_finish")
        typeImage.tintColor = UIColor.blackPrimaryText()
      }
    }
  }

  private func setupLabels() {
    let subtitle: String?
    if model.isMyPosition && index != 0 {
      titleLabel.text = model.latLonString
      subtitle = model.title
    } else {
      titleLabel.text = model.title
      subtitle = model.subtitle
    }
    var subtitleConstraintsActive = false
    if let subtitle = subtitle, !subtitle.isEmpty {
      subtitleLabel.text = subtitle
      subtitleConstraintsActive = true
    }
    subtitleLabel.isHidden = !subtitleConstraintsActive
    subtitleConstraints.forEach { $0.isActive = subtitleConstraintsActive }
  }

  private func setupSeparators() {
    let isSeparatorsHidden = model.type == .finish
    separator1.isHidden = isSeparatorsHidden
    separator2.isHidden = isSeparatorsHidden
  }
}
