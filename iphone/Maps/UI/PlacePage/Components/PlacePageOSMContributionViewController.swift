protocol PlacePageOSMContributionViewControllerDelegate: AnyObject {
  func didPressAddPlace()
  func didPressEditPlace()
  func didPressUpdateMap()
}

final class PlacePageOSMContributionViewController: UIViewController {

  private enum Constants {
    static let horizontalPadding: CGFloat = 16
    static let headerTopPadding: CGFloat = 12
    static let headerHeight: CGFloat = 24
    static let buttonsHeight: CGFloat = 44
    static let headerSpacing: CGFloat = 16
    static let buttonsTopSpacing: CGFloat = 12
    static let buttonsBottomPadding: CGFloat = 16
    static let buttonsSpacing: CGFloat = 12
  }

  private let osmLogoImageView = UIImageView()
  private let titleLabel = UILabel()
  private let addPlaceButton = UIButton(type: .system)
  private let editPlaceButton = UIButton(type: .system)
  private let updateMapButton = UIButton(type: .system)
  private let headerStackView = UIStackView()
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
    osmLogoImageView.contentMode = .scaleAspectFit
    osmLogoImageView.image = UIImage(resource: .osmLogo)

    titleLabel.text = L("contribute_to_osm")
    titleLabel.setFontStyle(.regular16, color: .blackPrimary)

    headerStackView.axis = .horizontal
    headerStackView.alignment = .center
    headerStackView.spacing = Constants.headerSpacing

    configureButton(addPlaceButton, withTitle: L("contribute_to_osm_add_place"), action: #selector(onAddPlace))
    configureButton(editPlaceButton, withTitle: L("contribute_to_osm_wrong_info"), action: #selector(onEditPlace))
    configureButton(updateMapButton, withTitle: L("contribute_to_osm_update_map"), action: #selector(onUpdateMap))

    buttonsStackView.axis = .horizontal
    buttonsStackView.spacing = Constants.buttonsSpacing
    buttonsStackView.distribution = .fillEqually
  }

  private func configureButton(_ button: UIButton, withTitle title: String, action: Selector) {
    button.setTitle(title, for: .normal)
    button.addTarget(self, action: action, for: .touchUpInside)
    button.setStyle(.flatNormalGrayButtonBig)
    button.titleLabel?.minimumScaleFactor = 0.5
    button.titleLabel?.adjustsFontSizeToFitWidth = true
    button.titleLabel?.numberOfLines = 2
  }

  private func layoutView() {
    view.addSubview(headerStackView)
    view.addSubview(buttonsStackView)
    headerStackView.addArrangedSubview(osmLogoImageView)
    headerStackView.addArrangedSubview(titleLabel)
    buttonsStackView.addArrangedSubview(addPlaceButton)
    buttonsStackView.addArrangedSubview(editPlaceButton)
    buttonsStackView.addArrangedSubview(updateMapButton)

    headerStackView.translatesAutoresizingMaskIntoConstraints = false
    buttonsStackView.translatesAutoresizingMaskIntoConstraints = false
    osmLogoImageView.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      headerStackView.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: Constants.horizontalPadding),
      headerStackView.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -Constants.horizontalPadding),
      headerStackView.topAnchor.constraint(equalTo: view.topAnchor, constant: Constants.headerTopPadding),
      headerStackView.heightAnchor.constraint(equalToConstant: Constants.headerHeight),

      osmLogoImageView.widthAnchor.constraint(equalToConstant: Constants.headerHeight),
      osmLogoImageView.heightAnchor.constraint(equalToConstant: Constants.headerHeight),

      buttonsStackView.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: Constants.horizontalPadding),
      buttonsStackView.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -Constants.horizontalPadding),
      buttonsStackView.topAnchor.constraint(equalTo: headerStackView.bottomAnchor, constant: Constants.buttonsTopSpacing),
      buttonsStackView.bottomAnchor.constraint(equalTo: view.bottomAnchor, constant: -Constants.buttonsBottomPadding),
      buttonsStackView.heightAnchor.constraint(equalToConstant: Constants.buttonsHeight)
    ])
  }

  private func updateButtonsVisibility() {
    UIView.transition(with: buttonsStackView, duration: kFastAnimationDuration, options: .transitionCrossDissolve) { [ weak self ] in
      guard let self = self else { return }
      addPlaceButton.isHidden = !buttonsData.showAddPlace
      editPlaceButton.isHidden = !buttonsData.showEditPlace
      updateMapButton.isHidden = !buttonsData.showUpdateMap
      addPlaceButton.isEnabled = buttonsData.enableAddPlace
      editPlaceButton.isEnabled = buttonsData.enableEditPlace
      updateMapButton.isEnabled = buttonsData.enableUpdateMap
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
}
