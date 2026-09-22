protocol SearchOnMapHeaderViewDelegate: UISearchBarDelegate {
  func cancelButtonDidTap()
  func grabberDidTap()
  func currentLocationButtonDidTap()
  func chooseOnMapButtonDidTap()
}

private final class RouteActionButton: UIButton {
  override func layoutSubviews() {
    super.layoutSubviews()
    if #available(iOS 26.0, *) {
      return
    }
    layer.cornerRadius = bounds.height / 2
  }
}

final class SearchOnMapHeaderView: UIView {
  weak var delegate: SearchOnMapHeaderViewDelegate? {
    didSet {
      searchBar.delegate = delegate
    }
  }

  private enum Constants {
    static let minSearchBarHeight: CGFloat = 36
    static let searchBarInsets: UIEdgeInsets = .init(top: 0, left: 10, bottom: 0, right: 0)
    static let grabberHeight: CGFloat = 5
    static let grabberWidth: CGFloat = 36
    static let grabberTopMargin: CGFloat = 5
    static let routeActionButtonSpacing: CGFloat = 8
    static let routeActionIconInset: CGFloat = 8
  }

  private let grabberView = UIView()
  private let grabberTapHandlerView = UIView()
  private let routeActionsStackView = UIStackView()
  private let currentLocationButton = RouteActionButton(type: .system)
  private let chooseOnMapButton = RouteActionButton(type: .system)
  private let searchBar = UISearchBar()
  private let cancelButton = SearchOnMapCancelButton()
  private var separator: UIView?
  private var searchBarLeadingDefaultConstraint: NSLayoutConstraint!
  private var searchBarLeadingWithRouteActionsConstraint: NSLayoutConstraint!
  private var hasRouteActions = false
  private var areRouteActionsVisible = false

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
    layoutView()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    setStyle(.background)

    setupGrabberView()
    setupGrabberTapHandlerView()
    setupRouteActionsView()
    setupSearchBar()
    setupCancelButton()
  }

  private func setupGrabberView() {
    grabberView.setStyle(.grabber)
    iPadSpecific { [weak self] in
      self?.grabberView.isHidden = true
    }
  }

  private func setupGrabberTapHandlerView() {
    grabberTapHandlerView.backgroundColor = .clear
    iPhoneSpecific {
      let tapGesture = UITapGestureRecognizer(target: self, action: #selector(grabberDidTap))
      grabberTapHandlerView.addGestureRecognizer(tapGesture)
    }
  }

  private func setupSearchBar() {
    searchBar.setStyle(.searchOnMapSearchBar)
    searchBar.placeholder = L("search")
    searchBar.showsCancelButton = false
    searchBar.searchBarStyle = .minimal
    searchBar.searchTextField.clearButtonMode = .always
    searchBar.returnKeyType = .search
    searchBar.searchTextField.enablesReturnKeyAutomatically = true
    // setSearchText only runs for programmatic fills, so typing needs its own signal.
    searchBar.searchTextField.addTarget(self, action: #selector(searchTextDidChange), for: .editingChanged)
  }

  private func setupRouteActionsView() {
    routeActionsStackView.axis = .horizontal
    routeActionsStackView.alignment = .center
    routeActionsStackView.spacing = Constants.routeActionButtonSpacing
    routeActionsStackView.alpha = 0
    routeActionsStackView.isHidden = true

    configureRouteActionButton(currentLocationButton,
                               image: UIImage(resource: .icCurrentPosition),
                               accessibilityLabel: L("p2p_your_location"),
                               action: #selector(currentLocationButtonDidTap))
    configureRouteActionButton(chooseOnMapButton,
                               image: UIImage(resource: .icSearchModeMap),
                               accessibilityLabel: L("choose_on_map"),
                               action: #selector(chooseOnMapButtonDidTap))

    routeActionsStackView.addArrangedSubview(currentLocationButton)
    routeActionsStackView.addArrangedSubview(chooseOnMapButton)
  }

  private func configureRouteActionButton(_ button: UIButton,
                                          image: UIImage,
                                          accessibilityLabel: String,
                                          action: Selector) {
    let image = image.withRenderingMode(.alwaysTemplate)
    if #available(iOS 26.0, *) {
      var configuration = UIButton.Configuration.glass()
      configuration.image = image
      configuration.baseForegroundColor = .linkBlue
      configuration.contentInsets = NSDirectionalEdgeInsets(top: Constants.routeActionIconInset,
                                                            leading: Constants.routeActionIconInset,
                                                            bottom: Constants.routeActionIconInset,
                                                            trailing: Constants.routeActionIconInset)
      button.configuration = configuration
    } else {
      button.backgroundColor = .pressBackground
      button.tintColor = .linkBlue
      button.clipsToBounds = true
      button.setImage(image, for: .normal)
      button.imageEdgeInsets = UIEdgeInsets(top: Constants.routeActionIconInset,
                                            left: Constants.routeActionIconInset,
                                            bottom: Constants.routeActionIconInset,
                                            right: Constants.routeActionIconInset)
    }
    button.accessibilityLabel = accessibilityLabel
    button.addTarget(self, action: action, for: .touchUpInside)
  }

  private func setupCancelButton() {
    cancelButton.addTarget(self, action: #selector(cancelButtonDidTap), for: .touchUpInside)
  }

  private func layoutView() {
    addSubview(grabberView)
    addSubview(grabberTapHandlerView)
    addSubview(cancelButton)
    addSubview(routeActionsStackView)
    addSubview(searchBar)

    if #available(iOS 26.0, *) {}
    else {
      separator = addSeparator(.bottom)
    }

    grabberView.translatesAutoresizingMaskIntoConstraints = false
    grabberTapHandlerView.translatesAutoresizingMaskIntoConstraints = false
    grabberTapHandlerView.setContentHuggingPriority(.defaultLow, for: .vertical)
    routeActionsStackView.translatesAutoresizingMaskIntoConstraints = false
    searchBar.translatesAutoresizingMaskIntoConstraints = false
    cancelButton.translatesAutoresizingMaskIntoConstraints = false
    currentLocationButton.translatesAutoresizingMaskIntoConstraints = false
    chooseOnMapButton.translatesAutoresizingMaskIntoConstraints = false

    searchBarLeadingDefaultConstraint = searchBar.leadingAnchor.constraint(equalTo: leadingAnchor,
                                                                           constant: Constants.searchBarInsets.left)
    searchBarLeadingWithRouteActionsConstraint = searchBar.leadingAnchor.constraint(equalTo: routeActionsStackView.trailingAnchor)

    NSLayoutConstraint.activate([
      grabberView.topAnchor.constraint(equalTo: topAnchor, constant: Constants.grabberTopMargin),
      grabberView.centerXAnchor.constraint(equalTo: centerXAnchor),
      grabberView.widthAnchor.constraint(equalToConstant: Constants.grabberWidth),
      grabberView.heightAnchor.constraint(equalToConstant: Constants.grabberHeight),

      grabberTapHandlerView.topAnchor.constraint(equalTo: grabberView.bottomAnchor),
      grabberTapHandlerView.leadingAnchor.constraint(equalTo: leadingAnchor),
      grabberTapHandlerView.trailingAnchor.constraint(equalTo: trailingAnchor),
      grabberTapHandlerView.bottomAnchor.constraint(equalTo: searchBar.topAnchor),

      routeActionsStackView.leadingAnchor.constraint(equalTo: leadingAnchor, constant: Constants.searchBarInsets.left),
      routeActionsStackView.centerYAnchor.constraint(equalTo: searchBar.searchTextField.centerYAnchor),

      currentLocationButton.heightAnchor.constraint(equalTo: searchBar.searchTextField.heightAnchor),
      currentLocationButton.widthAnchor.constraint(equalTo: currentLocationButton.heightAnchor),
      chooseOnMapButton.heightAnchor.constraint(equalTo: searchBar.searchTextField.heightAnchor),
      chooseOnMapButton.widthAnchor.constraint(equalTo: chooseOnMapButton.heightAnchor),

      searchBar.topAnchor.constraint(greaterThanOrEqualTo: safeAreaLayoutGuide.topAnchor, constant: Constants.searchBarInsets.top),
      searchBar.topAnchor.constraint(equalTo: grabberView.bottomAnchor, constant: Constants.searchBarInsets.top).withPriority(.defaultLow),
      searchBarLeadingDefaultConstraint,
      searchBar.trailingAnchor.constraint(equalTo: cancelButton.leadingAnchor),
      searchBar.bottomAnchor.constraint(equalTo: bottomAnchor, constant: -Constants.searchBarInsets.bottom),
      searchBar.heightAnchor.constraint(greaterThanOrEqualToConstant: Constants.minSearchBarHeight),

      cancelButton.topAnchor.constraint(equalTo: searchBar.topAnchor),
      cancelButton.trailingAnchor.constraint(equalTo: trailingAnchor),
      cancelButton.bottomAnchor.constraint(equalTo: searchBar.bottomAnchor),
    ])
  }

  @objc private func grabberDidTap() {
    delegate?.grabberDidTap()
  }

  @objc private func cancelButtonDidTap() {
    delegate?.cancelButtonDidTap()
  }

  @objc private func currentLocationButtonDidTap() {
    delegate?.currentLocationButtonDidTap()
  }

  @objc private func chooseOnMapButtonDidTap() {
    delegate?.chooseOnMapButtonDidTap()
  }

  @objc private func searchTextDidChange() {
    updateRouteActionButtonsVisibility()
  }

  func setSearchText(_ text: String) {
    searchBar.text = text
    updateRouteActionButtonsVisibility()
  }

  func setIsSearching(_ isSearching: Bool) {
    if isSearching {
      searchBar.becomeFirstResponder()
    } else if searchBar.isFirstResponder {
      searchBar.resignFirstResponder()
    }
  }

  func setRoutePointActions(_ actions: SearchOnMap.ViewModel.RoutePointActions?) {
    hasRouteActions = actions != nil
    searchBar.placeholder = actions?.title ?? L("search")
    currentLocationButton.isHidden = actions?.canSelectCurrentLocation != true
    chooseOnMapButton.isHidden = actions == nil
    updateRouteActionButtonsVisibility()
  }

  var searchQuery: SearchQuery {
    SearchQuery(searchBar.text ?? "", locale: searchBar.textInputMode?.primaryLanguage, source: .typedText)
  }

  func setSeparatorHidden(_ hidden: Bool) {
    separator?.isHidden = hidden
  }

  private func updateRouteActionButtonsVisibility() {
    let shouldShow = hasRouteActions && (searchBar.text?.isEmpty ?? true)
    guard areRouteActionsVisible != shouldShow else { return }

    areRouteActionsVisible = shouldShow
    if shouldShow {
      routeActionsStackView.isHidden = false
    }

    UIView.animate(withDuration: AppConstants.fastAnimationDuration,
                   delay: 0,
                   options: .curveEaseInOut,
                   animations: {
                     self.routeActionsStackView.alpha = shouldShow ? 1 : 0
                     NSLayoutConstraint.deactivate([
                       self.searchBarLeadingDefaultConstraint,
                       self.searchBarLeadingWithRouteActionsConstraint,
                     ])
                     NSLayoutConstraint.activate([
                       shouldShow ? self.searchBarLeadingWithRouteActionsConstraint
                         : self.searchBarLeadingDefaultConstraint,
                     ])
                     self.layoutIfNeeded()
                   },
                   // An interrupted animation must not settle the stack to its own outdated target.
                   completion: { _ in self.routeActionsStackView.isHidden = !self.areRouteActionsVisible })
  }
}
