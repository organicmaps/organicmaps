final class RouteEstimatesPreviewView: SolidTouchView {

  private enum Constants {
    static let contentInsets = UIEdgeInsets(top: 8, left: 0, bottom: -8, right: 0)
    static let spacing: CGFloat = 8
  }

  private let stackView = UIStackView()
  private var etaLabel = UILabel()
  private var stepsCollectionView: TransportTransitStepsCollectionView!
  private var stepsCollectionViewHeight: NSLayoutConstraint!

  weak var navigationInfo: MWMNavigationDashboardEntity?

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

    etaLabel.setFontStyleAndApply(.bold14)

    stackView.axis = .vertical
    stackView.spacing = Constants.spacing
    stackView.addArrangedSubview(etaLabel)
    stackView.addArrangedSubview(stepsCollectionView)
  }

  private func layout() {
    addSubview(stackView)

    stackView.translatesAutoresizingMaskIntoConstraints = false

    stepsCollectionViewHeight = stepsCollectionView.heightAnchor.constraint(equalToConstant: 0).withPriority(.defaultHigh)

    NSLayoutConstraint.activate([
      stackView.topAnchor.constraint(equalTo: topAnchor, constant: Constants.contentInsets.top),
      stackView.leadingAnchor.constraint(equalTo: leadingAnchor, constant: Constants.contentInsets.left),
      stackView.trailingAnchor.constraint(equalTo: trailingAnchor, constant: Constants.contentInsets.right),
      stackView.bottomAnchor.constraint(equalTo: bottomAnchor, constant: Constants.contentInsets.bottom),
      stepsCollectionViewHeight
    ])
    layoutSubviews()
  }

  // TODO: implement show/hide the steps collection
  // TODO: move settings buttom here
  func onNavigationInfoUpdated(_ viewModel: RoutePreview.ViewModel) {
    navigationInfo = viewModel.entity
    etaLabel.attributedText = viewModel.estimates

    if let steps = viewModel.entity.transitSteps {
      stepsCollectionView.steps = steps
      stepsCollectionView.isHidden = steps.isEmpty
    } else {
      stepsCollectionView.isHidden = true
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

  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return .bottom
  }

  override var visibleAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return .bottom
  }

  override var widgetsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    return .bottom
  }
}
