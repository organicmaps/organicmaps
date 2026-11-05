final class TransportTransitStepsView: SolidTouchView {
  private enum Constants {
    static let contentInsets = UIEdgeInsets(top: 8, left: 0, bottom: -8, right: 0)
  }

  private var stepsCollectionView: TransportTransitStepsCollectionView!
  private var stepsCollectionViewHeight: NSLayoutConstraint!

  var onContentHeightChanged: (() -> Void)?

  init() {
    super.init(frame: .zero)
    setupViews()
    layout()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupViews() {
    let layout = TransportTransitFlowLayout()
    stepsCollectionView = TransportTransitStepsCollectionView(layout: layout)
    stepsCollectionView.backgroundColor = .clear
    stepsCollectionView.isScrollEnabled = false
    stepsCollectionView.onContentHeightChanged = { [weak self] height in
      guard let self else { return }
      self.stepsCollectionViewHeight.constant = height
      self.onContentHeightChanged?()
    }
  }

  private func layout() {
    addSubview(stepsCollectionView)
    stepsCollectionView.translatesAutoresizingMaskIntoConstraints = false

    stepsCollectionViewHeight = stepsCollectionView.heightAnchor.constraint(equalToConstant: 0)

    NSLayoutConstraint.activate([
      stepsCollectionView.topAnchor.constraint(equalTo: topAnchor, constant: Constants.contentInsets.top),
      stepsCollectionView.leadingAnchor.constraint(equalTo: leadingAnchor, constant: Constants.contentInsets.left),
      stepsCollectionView.trailingAnchor.constraint(equalTo: trailingAnchor, constant: Constants.contentInsets.right),
      // A hidden arranged view collapses to zero height, including its insets.
      stepsCollectionView.bottomAnchor.constraint(equalTo: bottomAnchor, constant: Constants.contentInsets.bottom)
        .withPriority(UILayoutPriority(999)),
      stepsCollectionViewHeight,
    ])
  }

  func setNavigationInfo(_ navigationInfo: MWMNavigationDashboardEntity?) {
    let steps = navigationInfo?.transitSteps ?? []
    stepsCollectionView.steps = steps
    isHidden = steps.isEmpty
  }
}
