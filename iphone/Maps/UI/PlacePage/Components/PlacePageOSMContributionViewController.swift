protocol PlacePageOSMContributionViewControllerDelegate: AnyObject {
  func didPressAddPlace()
  func didPressEditPlace()
  func didPressUpdateMap()
  func didPressOSMInfo()
}

final class PlacePageOSMContributionViewController: UIViewController {

  private enum Constants {
    static let horizontalPadding: CGFloat = 16
    static let headerHeight: CGFloat = 44
    static let buttonsHeight: CGFloat = 44
    static let buttonsTopSpacing: CGFloat = 12
    static let buttonsBottomPadding: CGFloat = 16
    static let buttonsSpacing: CGFloat = 12
  }

  private let osmHeaderView = InfoItemView()
  private let addPlaceButton = UIButton(type: .system)
  private let editPlaceButton = UIButton(type: .system)
  private let updateMapButton = UIButton(type: .system)
  private let buttonsStackView = UIStackView()

  var buttonsData: PlacePageOSMContributionData {
    didSet {
      updateButtonsVisibility()
    }
  }
  private weak var delegate: PlacePageOSMContributionViewControllerDelegate?

  init(data: PlacePageOSMContributionData, delegate: PlacePageOSMContributionViewControllerDelegate?) {
    self.buttonsData = data
    self.delegate = delegate
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    setupView()
    layoutView()
    updateButtonsVisibility()
  }

  private func setupView() {
    view.setStyle(.background)

    osmHeaderView.setIcon(image: UIImage(resource: .osmLogo))
    osmHeaderView.setTitle(L("contribute_to_osm"))
    osmHeaderView.setAccessory(image: UIImage(resource: .icQuestionmark),
                               tapHandler: { [weak self] in
      self?.onOSMInfo()
    })

    configureButton(addPlaceButton, enabledTitle: L("contribute_to_osm_add_place"), action: #selector(onAddPlace))
    configureButton(editPlaceButton, enabledTitle: L("edit_place"), action: #selector(onEditPlace))
    configureButton(updateMapButton, enabledTitle: L("contribute_to_osm_update_map"), disabledTitle: L("downloading"), action: #selector(onUpdateMap))

    buttonsStackView.axis = .horizontal
    buttonsStackView.spacing = Constants.buttonsSpacing
    buttonsStackView.distribution = .fillEqually
  }

  private func configureButton(_ button: UIButton,
                               enabledTitle: String,
                               disabledTitle: String? = nil,
                               action: Selector) {
    button.setTitle(enabledTitle, for: .normal)
    if let disabledTitle {
      button.setTitle(disabledTitle, for: .disabled)
    }
    button.addTarget(self, action: action, for: .touchUpInside)
    button.setStyle(.flatNormalGrayButtonBig)
    button.contentEdgeInsets = UIEdgeInsets(top: 0, left: 8, bottom: 0, right: 8)
    button.titleLabel?.minimumScaleFactor = 0.5
    button.titleLabel?.adjustsFontSizeToFitWidth = true
    button.titleLabel?.allowsDefaultTighteningForTruncation = true
    button.titleLabel?.numberOfLines = 1
  }

  private func layoutView() {
    view.addSubview(osmHeaderView)
    view.addSubview(buttonsStackView)
    buttonsStackView.addArrangedSubview(addPlaceButton)
    buttonsStackView.addArrangedSubview(editPlaceButton)
    buttonsStackView.addArrangedSubview(updateMapButton)

    osmHeaderView.translatesAutoresizingMaskIntoConstraints = false
    buttonsStackView.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      osmHeaderView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      osmHeaderView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      osmHeaderView.topAnchor.constraint(equalTo: view.topAnchor),
      osmHeaderView.heightAnchor.constraint(equalToConstant: Constants.headerHeight),

      buttonsStackView.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: Constants.horizontalPadding),
      buttonsStackView.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -Constants.horizontalPadding),
      buttonsStackView.topAnchor.constraint(equalTo: osmHeaderView.bottomAnchor, constant: Constants.buttonsTopSpacing),
      buttonsStackView.bottomAnchor.constraint(equalTo: view.bottomAnchor, constant: -Constants.buttonsBottomPadding),
      buttonsStackView.heightAnchor.constraint(equalToConstant: Constants.buttonsHeight)
    ])
  }

  private func updateButtonsVisibility() {
    switch buttonsData.state {
    case .canAddOrEditPlace:
      addPlaceButton.isHidden = !buttonsData.showAddPlace
      editPlaceButton.isHidden = !buttonsData.showEditPlace
      updateMapButton.isHidden = true
    case .shouldUpdateMap:
      addPlaceButton.isHidden = true
      editPlaceButton.isHidden = true
      updateMapButton.isHidden = false
      updateMapButton.isEnabled = true
    case .mapIsDownloading:
      addPlaceButton.isHidden = true
      editPlaceButton.isHidden = true
      updateMapButton.isHidden = false
      updateMapButton.isEnabled = false
    @unknown default:
      fatalError("Unknown state: \(buttonsData.state)")
    }
  }

  // MARK: - Actions

  @objc private func onAddPlace() {
    delegate?.didPressAddPlace()
  }

  @objc private func onEditPlace() {
    delegate?.didPressEditPlace()
  }

  @objc private func onUpdateMap() {
    delegate?.didPressUpdateMap()
  }

  @objc private func onOSMInfo() {
    delegate?.didPressOSMInfo()
  }
}
