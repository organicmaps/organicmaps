
@objc class PromoButton: MWMButton {

  private let coordinator: PromoCoordinator
  private let buttonSize: CGSize = CGSize(width: 48, height: 48)

  @objc init(coordinator: PromoCoordinator) {
    self.coordinator = coordinator
    super.init(frame: CGRect(x: 0, y: 0, width: buttonSize.width, height: buttonSize.height))

    let view = UIView(frame: CGRect(x: buttonSize.width - 14, y: 0, width: 12, height: 12))
    view.layer.cornerRadius = view.size.width/2
    view.backgroundColor = UIColor.red
    addSubview(view)

    configure()
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func configure() {
    removeTarget(self, action: nil, for: .touchUpInside)
    configureDiscovery()
  }

  private func configureDiscovery() {
    setStyleAndApply("PromoDiscroveryButton")
    addTarget(self, action: #selector(onButtonPress), for: .touchUpInside)

    imageView?.clipsToBounds = false
    imageView?.contentMode = .center

    let animation = CABasicAnimation(keyPath: "transform.rotation.z")
    animation.duration = kDefaultAnimationDuration
    animation.fromValue = 0
    animation.toValue = Double(-30 * Double.pi / 180)
    animation.autoreverses = true
    animation.timingFunction = CAMediaTimingFunction(name: CAMediaTimingFunctionName.easeInEaseOut)
    animation.repeatCount = 1

    let animationGroup = CAAnimationGroup()
    animationGroup.duration = 3
    animationGroup.repeatCount = Float(Int.max)
    animationGroup.animations = [animation]
    animationGroup.isRemovedOnCompletion = false
    imageView?.layer.add(animationGroup, forKey: "transform.rotation.z")
  }

  @objc private func onButtonPress(sender: UIButton) {
    coordinator.onPromoButtonPress(completion: { [weak self] in
      ABTestManager.manager().promoDiscoveryCampaign.onActivate();
      self?.isHidden = true;
    })
  }
}
