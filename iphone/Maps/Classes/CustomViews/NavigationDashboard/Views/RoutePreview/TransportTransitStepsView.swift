final class TransportTransitStepsView: SolidTouchView {

  private enum Constants {
    static let contentInsets = UIEdgeInsets(top: 8, left: 0, bottom: -8, right: 0)
    static let spacing: CGFloat = 8
  }

  private var stepsCollectionView: TransportTransitStepsCollectionView!
  private var stepsCollectionViewHeight: NSLayoutConstraint!

  init() {
    super.init(frame: .zero)
    setupViews()
    layout()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setupViews()
  }

  private func setupViews() {
    let layout = TransportTransitFlowLayout()
    stepsCollectionView = TransportTransitStepsCollectionView(layout: layout)
  }

  private func layout() {
    addSubview(stepsCollectionView)
    stepsCollectionView.translatesAutoresizingMaskIntoConstraints = false

    stepsCollectionViewHeight = stepsCollectionView.heightAnchor.constraint(equalToConstant: 0).withPriority(.defaultHigh)

    NSLayoutConstraint.activate([
      stepsCollectionView.topAnchor.constraint(equalTo: topAnchor, constant: Constants.contentInsets.top),
      stepsCollectionView.leadingAnchor.constraint(equalTo: leadingAnchor, constant: Constants.contentInsets.left),
      stepsCollectionView.trailingAnchor.constraint(equalTo: trailingAnchor, constant: Constants.contentInsets.right),
      stepsCollectionView.bottomAnchor.constraint(equalTo: bottomAnchor, constant: Constants.contentInsets.bottom),
      stepsCollectionViewHeight
    ])
    layoutSubviews()
  }

  func setNavigationInfo(_ navigationInfo: MWMNavigationDashboardEntity) {
    if let steps = navigationInfo.transitSteps {
      stepsCollectionView.steps = steps
      isHidden = steps.isEmpty
    } else {
      isHidden = true
    }
    updateHeight()
  }

  private func updateHeight() {
    stepsCollectionView.layoutIfNeeded()
    DispatchQueue.main.async {
      self.animateConstraints(animations: {
        self.stepsCollectionViewHeight.constant = self.stepsCollectionView.collectionViewLayout.collectionViewContentSize.height
      })
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    layoutSubviews()
  }
}
