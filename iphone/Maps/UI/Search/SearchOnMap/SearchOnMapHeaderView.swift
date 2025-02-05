protocol SearchOnMapHeaderViewDelegate: UISearchBarDelegate {
  func cancelButtonDidTap()
}

final class SearchOnMapHeaderView: UIView {
  weak var delegate: SearchOnMapHeaderViewDelegate? {
    didSet {
      searchBar.delegate = delegate
    }
  }

  private enum Constants {
    static let grabberHeight: CGFloat = 5
    static let grabberWidth: CGFloat = 36
    static let grabberTopMargin: CGFloat = 5
    static let cancelButtonInsets: UIEdgeInsets = UIEdgeInsets(top: 0, left: 6, bottom: 0, right: 8)
  }

  private let grabberView = UIView()
  private let searchBar = UISearchBar()
  private let cancelButton = UIButton()

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
    layoutView()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    setStyle(.searchHeader)
    layer.maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]

    setupGrabberView()
    setupSearchBar()
    setupCancelButton()
  }

  private func setupGrabberView() {
    grabberView.setStyle(.background)
    grabberView.layer.setCorner(radius: Constants.grabberHeight / 2)
    iPadSpecific { [weak self] in
      self?.grabberView.isHidden = true
    }
  }

  private func setupSearchBar() {
    searchBar.placeholder = L("search")
    searchBar.showsCancelButton = false
    if #available(iOS 13.0, *) {
      searchBar.searchTextField.clearButtonMode = .always
      searchBar.returnKeyType = .search
      searchBar.searchTextField.enablesReturnKeyAutomatically = true
    }
  }

  private func setupCancelButton() {
    cancelButton.tintColor = .whitePrimaryText()
    cancelButton.setStyle(.clearBackground)
    cancelButton.setTitle(L("cancel"), for: .normal)
    cancelButton.addTarget(self, action: #selector(cancelButtonTapped), for: .touchUpInside)
  }

  private func layoutView() {
    addSubview(grabberView)
    addSubview(searchBar)
    addSubview(cancelButton)

    grabberView.translatesAutoresizingMaskIntoConstraints = false
    searchBar.translatesAutoresizingMaskIntoConstraints = false
    cancelButton.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      grabberView.topAnchor.constraint(equalTo: topAnchor, constant: Constants.grabberTopMargin),
      grabberView.centerXAnchor.constraint(equalTo: centerXAnchor),
      grabberView.widthAnchor.constraint(equalToConstant: Constants.grabberWidth),
      grabberView.heightAnchor.constraint(equalToConstant: Constants.grabberHeight),

      searchBar.topAnchor.constraint(equalTo: grabberView.bottomAnchor),
      searchBar.leadingAnchor.constraint(equalTo: leadingAnchor),
      searchBar.trailingAnchor.constraint(equalTo: cancelButton.leadingAnchor, constant: -Constants.cancelButtonInsets.left),

      cancelButton.centerYAnchor.constraint(equalTo: searchBar.centerYAnchor),
      cancelButton.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -Constants.cancelButtonInsets.right),

      bottomAnchor.constraint(equalTo: searchBar.bottomAnchor)
    ])
  }

  @objc private func cancelButtonTapped() {
    delegate?.cancelButtonDidTap()
  }

  func setSearchText(_ text: String) {
    searchBar.text = text
  }

  func setIsSearching(_ isSearching: Bool) {
    if isSearching {
      searchBar.becomeFirstResponder()
    } else if searchBar.isFirstResponder {
      searchBar.resignFirstResponder()
    }
  }

  var searchText: SearchOnMap.SearchText {
    SearchOnMap.SearchText(searchBar.text ?? "", locale: searchBar.textInputMode?.primaryLanguage)
  }
}
