protocol SearchOnMapView: AnyObject {
  var scrollViewDelegate: SearchOnMapScrollViewDelegate? { get set }

  func render(_ viewModel: SearchOnMap.ViewModel)
  func close()
}

@objc
protocol SearchOnMapScrollViewDelegate: AnyObject {
  func scrollViewDidScroll(_ scrollView: UIScrollView)
}

final class SearchOnMapViewController: UIViewController {
  typealias ViewModel = SearchOnMap.ViewModel
  typealias ContentState = SearchOnMap.ViewModel.ContentState
  typealias SearchText = SearchOnMap.SearchText

  fileprivate enum Constants {
    static let estimatedRowHeight: CGFloat = 80
  }

  var interactor: SearchOnMapInteractor?
  var modalPresentationController: SearchOnMapPresentationController?
  weak var scrollViewDelegate: SearchOnMapScrollViewDelegate?
  
  private var searchResults = SearchOnMap.SearchResults([])

  // MARK: - UI Elements
  let contentView = UIView()
  private let headerView = SearchOnMapHeaderView()
  private let searchResultsView = UIView()
  private let resultsTableView = UITableView()
  private let historyAndCategoryTabViewController = SearchTabViewController()
  private var searchingActivityView = PlaceholderView(hasActivityIndicator: true)
  private var containerModalYTranslation: CGFloat = 0
  private var searchNoResultsView = PlaceholderView(title: L("search_not_found"),
                                                    subtitle: L("search_not_found_query"))

  // MARK: - Init
  init(presentationController: SearchOnMapPresentationController?) {
    self.modalPresentationController = presentationController
    super.init(nibName: nil, bundle: nil)
    modalPresentationController?.setViewController(self)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  // MARK: - Lifecycle
  override func loadView() {
    view = SearchOnMapAreaView()
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    setupViews()
    layoutViews()
    modalPresentationController?.show()
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    headerView.setIsSearching(false)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    modalPresentationController?.traitCollectionDidChange(traitCollection)
  }

  // MARK: - Private methods
  private func setupViews() {
    contentView.setStyle(.modalSheetContent)
    setupTapGestureRecognizer()
    setupHeaderView()
    setupContainerView()
    setupResultsTableView()
    setupHistoryAndCategoryTabView()
    setupResultsTableView()
  }

  private func setupTapGestureRecognizer() {
    let tapGesture = UITapGestureRecognizer(target: self, action: #selector(handleTapOutside))
    tapGesture.cancelsTouchesInView = false
    view.addGestureRecognizer(tapGesture)
  }

  private func setupHeaderView() {
    headerView.delegate = self
  }

  private func setupContainerView() {
    searchResultsView.setStyle(.background)
  }

  private func setupResultsTableView() {
    resultsTableView.setStyle(.background)
    resultsTableView.estimatedRowHeight = Constants.estimatedRowHeight
    resultsTableView.rowHeight = UITableView.automaticDimension
    resultsTableView.registerNib(cellClass: SearchSuggestionCell.self)
    resultsTableView.registerNib(cellClass: SearchCommonCell.self)
    resultsTableView.dataSource = self
    resultsTableView.delegate = self
    resultsTableView.keyboardDismissMode = .onDrag
  }

  private func setupHistoryAndCategoryTabView() {
    historyAndCategoryTabViewController.delegate = self
  }

  private func layoutViews() {
    view.addSubview(contentView)
    contentView.addSubview(headerView)
    contentView.addSubview(searchResultsView)

    contentView.translatesAutoresizingMaskIntoConstraints = false
    headerView.translatesAutoresizingMaskIntoConstraints = false
    searchResultsView.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      contentView.topAnchor.constraint(equalTo: view.topAnchor),
      contentView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      contentView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      contentView.bottomAnchor.constraint(equalTo: view.bottomAnchor),

      headerView.topAnchor.constraint(equalTo: contentView.topAnchor),
      headerView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor),
      headerView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor),

      searchResultsView.topAnchor.constraint(equalTo: headerView.bottomAnchor),
      searchResultsView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor),
      searchResultsView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor),
      searchResultsView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor),
    ])

    layoutResultsView()
    layoutHistoryAndCategoryTabView()
    layoutSearchNoResultsView()
    layoutSearchingView()
  }

  private func layoutResultsView() {
    searchResultsView.addSubview(resultsTableView)
    resultsTableView.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      resultsTableView.topAnchor.constraint(equalTo: searchResultsView.topAnchor),
      resultsTableView.leadingAnchor.constraint(equalTo: searchResultsView.leadingAnchor),
      resultsTableView.trailingAnchor.constraint(equalTo: searchResultsView.trailingAnchor),
      resultsTableView.bottomAnchor.constraint(equalTo: searchResultsView.bottomAnchor)
    ])
  }

  private func layoutHistoryAndCategoryTabView() {
    searchResultsView.addSubview(historyAndCategoryTabViewController.view)
    historyAndCategoryTabViewController.view.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      historyAndCategoryTabViewController.view.topAnchor.constraint(equalTo: searchResultsView.topAnchor),
      historyAndCategoryTabViewController.view.leadingAnchor.constraint(equalTo: searchResultsView.leadingAnchor),
      historyAndCategoryTabViewController.view.trailingAnchor.constraint(equalTo: searchResultsView.trailingAnchor),
      historyAndCategoryTabViewController.view.bottomAnchor.constraint(equalTo: searchResultsView.bottomAnchor)
    ])
  }

  private func layoutSearchNoResultsView() {
    searchNoResultsView.translatesAutoresizingMaskIntoConstraints = false
    searchResultsView.addSubview(searchNoResultsView)
    NSLayoutConstraint.activate([
      searchNoResultsView.topAnchor.constraint(equalTo: searchResultsView.topAnchor),
      searchNoResultsView.leadingAnchor.constraint(equalTo: searchResultsView.leadingAnchor),
      searchNoResultsView.trailingAnchor.constraint(equalTo: searchResultsView.trailingAnchor),
      searchNoResultsView.bottomAnchor.constraint(equalTo: searchResultsView.bottomAnchor)
    ])
  }

  private func layoutSearchingView() {
    searchResultsView.insertSubview(searchingActivityView, at: 0)
    searchingActivityView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      searchingActivityView.leadingAnchor.constraint(equalTo: searchResultsView.leadingAnchor),
      searchingActivityView.trailingAnchor.constraint(equalTo: searchResultsView.trailingAnchor),
      searchingActivityView.topAnchor.constraint(equalTo: searchResultsView.topAnchor),
      searchingActivityView.bottomAnchor.constraint(equalTo: searchResultsView.bottomAnchor)
    ])
  }

  // MARK: - Handle Button Actions
  @objc private func handleTapOutside(_ gesture: UITapGestureRecognizer) {
    let location = gesture.location(in: view)
    if resultsTableView.frame.contains(location) && searchResults.isEmpty {
      headerView.setIsSearching(false)
    }
  }

  // MARK: - Handle State Updates
  private func setContent(_ content: ContentState) {
    switch content {
    case .historyAndCategory:
      historyAndCategoryTabViewController.reloadSearchHistory()
    case let .results(results):
      if searchResults != results {
        searchResults = results
        resultsTableView.reloadData()
      }
    case .noResults:
      searchResults = .empty
      resultsTableView.reloadData()
    case .searching:
      break
    }
    showView(viewToShow(for: content))
  }

  private func viewToShow(for content: ContentState) -> UIView {
    switch content {
    case .historyAndCategory:
      return historyAndCategoryTabViewController.view
    case .results:
      return resultsTableView
    case .noResults:
      return searchNoResultsView
    case .searching:
      return searchingActivityView
    }
  }

  private func showView(_ view: UIView) {
    let viewsToHide: [UIView] = [resultsTableView,
                                 historyAndCategoryTabViewController.view,
                                 searchNoResultsView,
                                 searchingActivityView].filter { $0 != view }
    UIView.transition(with: searchResultsView,
                      duration: kDefaultAnimationDuration / 2,
                      options: [.transitionCrossDissolve, .curveEaseInOut], animations: {
      viewsToHide.forEach { viewToHide in
        view.isHidden = false
        view.alpha = 1
        viewToHide.isHidden = true
        viewToHide.alpha = 0
      }
    })
  }

  private func setIsSearching(_ isSearching: Bool) {
    headerView.setIsSearching(isSearching)
  }

  private func replaceSearchText(with text: String) {
    headerView.setSearchText(text)
  }
}

// MARK: - Public methods
extension SearchOnMapViewController: SearchOnMapView {
  func render(_ viewModel: ViewModel) {
    setContent(viewModel.contentState)
    setIsSearching(viewModel.isTyping)
    if let searchingText = viewModel.searchingText {
      replaceSearchText(with: searchingText)
    }
    modalPresentationController?.setPresentationStep(viewModel.presentationStep)
  }

  func close() {
    headerView.setIsSearching(false)
    guard let modalPresentationController else {
      dismiss(animated: true)
      return
    }
    modalPresentationController.close()
  }
}

// MARK: - ModallyPresentedViewController
extension SearchOnMapViewController: ModallyPresentedViewController {
  func translationYDidUpdate(_ translationY: CGFloat) {
    self.containerModalYTranslation = translationY
    resultsTableView.contentInset.bottom = translationY
    historyAndCategoryTabViewController.translationYDidUpdate(translationY)
    searchNoResultsView.translationYDidUpdate(translationY)
    searchingActivityView.translationYDidUpdate(translationY)
  }
}

// MARK: - UITableViewDataSource
extension SearchOnMapViewController: UITableViewDataSource {
  func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    searchResults.count
  }

  func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let result = searchResults[indexPath.row]
    switch result.itemType {
    case .regular:
      let cell = tableView.dequeueReusableCell(cell: SearchCommonCell.self, indexPath: indexPath)
      cell.configure(with: result, isPartialMatching: searchResults.hasPartialMatch)
      return cell
    case .suggestion:
      let cell = tableView.dequeueReusableCell(cell: SearchSuggestionCell.self, indexPath: indexPath)
      cell.configure(with: result, isPartialMatching: true)
      cell.isLastCell = indexPath.row == searchResults.suggestionsCount - 1
      return cell
    @unknown default:
      fatalError("Unknown item type")
    }
  }
}

// MARK: - UITableViewDelegate
extension SearchOnMapViewController: UITableViewDelegate {
  func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    let result = searchResults[indexPath.row]
    interactor?.handle(.didSelectResult(result, withSearchText: headerView.searchText))
    tableView.deselectRow(at: indexPath, animated: true)
  }

  func scrollViewWillBeginDragging(_ scrollView: UIScrollView) {
    interactor?.handle(.didStartDraggingSearch)
  }

  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    scrollViewDelegate?.scrollViewDidScroll(scrollView)
  }
}

// MARK: - UICollectionViewDataSource
extension SearchOnMapViewController: UICollectionViewDataSource {
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    // TODO: remove search from here
    Int(Search.resultsCount())
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(withReuseIdentifier: "FilterCell", for: indexPath)
    return cell
  }
}

// MARK: - SearchOnMapHeaderViewDelegate
extension SearchOnMapViewController: SearchOnMapHeaderViewDelegate {
  func searchBarTextDidBeginEditing(_ searchBar: UISearchBar) {
    interactor?.handle(.didStartTyping)
  }

  func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
    guard !searchText.isEmpty else {
      interactor?.handle(.clearButtonDidTap)
      return
    }
    interactor?.handle(.didType(SearchText(searchText, locale: searchBar.textInputMode?.primaryLanguage)))
  }

  func searchBarSearchButtonClicked(_ searchBar: UISearchBar) {
    guard let searchText = searchBar.text, !searchText.isEmpty else { return }
    interactor?.handle(.searchButtonDidTap(SearchText(searchText, locale: searchBar.textInputMode?.primaryLanguage)))
  }

  func cancelButtonDidTap() {
    interactor?.handle(.closeSearch)
  }
}

// MARK: - SearchTabViewControllerDelegate
extension SearchOnMapViewController: SearchTabViewControllerDelegate {
  func searchTabController(_ viewController: SearchTabViewController, didSearch text: String, withCategory: Bool) {
    interactor?.handle(.didSelectText(SearchText(text, locale: nil), isCategory: withCategory))
  }
}

private class SearchOnMapAreaView: UIView {
  override var sideButtonsAreaAffectDirections: MWMAvailableAreaAffectDirections {
    alternative(iPhone: .bottom, iPad: [])
  }

  override var trafficButtonAreaAffectDirections: MWMAvailableAreaAffectDirections {
    alternative(iPhone: .bottom, iPad: [])
  }
}
