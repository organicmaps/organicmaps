protocol SearchOnMapView: AnyObject {
  var scrollViewDelegate: SearchOnMapScrollViewDelegate? { get set }

  func render(_ viewModel: SearchOnMap.ViewModel)
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
    static let categoriesHeight: CGFloat = 100
    static let filtersHeight: CGFloat = 50
    static let keyboardAnimationDuration: CGFloat = 0.3
    static let cancelButtonInsets: UIEdgeInsets = UIEdgeInsets(top: 0, left: 6, bottom: 0, right: 8)
    static let estimatedRowHeight: CGFloat = 80
  }

  let interactor: SearchOnMapInteractor
  weak var scrollViewDelegate: SearchOnMapScrollViewDelegate?

  private var searchResults = SearchOnMap.SearchResults([])

  // MARK: - UI Elements
  private let headerView = SearchOnMapHeaderView()
  private let containerView = UIView()
  private let resultsTableView = UITableView()
  private let historyAndCategoryTabViewController = SearchTabViewController()
  // TODO: implement filters
  private let filtersCollectionView: UICollectionView = {
    let layout = UICollectionViewFlowLayout()
    layout.scrollDirection = .horizontal
    return UICollectionView(frame: .zero, collectionViewLayout: layout)
  }()
  private var searchingActivityView = PlaceholderView(hasActivityIndicator: true)
  private var containerModalYTranslation: CGFloat = 0
  private var searchNoResultsView = PlaceholderView(title: L("search_not_found"),
                                                    subtitle: L("search_not_found_query"))

  // MARK: - Init
  init(interactor: SearchOnMapInteractor) {
    self.interactor = interactor
    super.init(nibName: nil, bundle: nil)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  deinit {
    NotificationCenter.default.removeObserver(self)
  }

  // MARK: - Lifecycle
  override func viewDidLoad() {
    super.viewDidLoad()
    setupViews()
    layoutViews()
    interactor.handle(.openSearch)
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    headerView.setIsSearching(false)
  }

  // MARK: - Private methods
  private func setupViews() {
    view.setStyle(.clearBackground)
    setupTapGestureRecognizer()
    setupHeaderView()
    setupContainerView()
    setupResultsTableView()
    setupHistoryAndCategoryTabView()
    setupResultsTableView()
    setupFiltersCollectionView()
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
    containerView.setStyle(.background)
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

  // TODO: (KK) Implement filters collection viewe
  private func setupFiltersCollectionView() {
    filtersCollectionView.register(UICollectionViewCell.self, forCellWithReuseIdentifier: "FilterCell")
    filtersCollectionView.dataSource = self
  }

  private func layoutViews() {
    view.addSubview(headerView)
    view.addSubview(containerView)
    headerView.translatesAutoresizingMaskIntoConstraints = false
    containerView.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      headerView.topAnchor.constraint(equalTo: view.topAnchor),
      headerView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      headerView.trailingAnchor.constraint(equalTo: view.trailingAnchor),

      containerView.topAnchor.constraint(equalTo: headerView.bottomAnchor),
      containerView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      containerView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      containerView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
    ])

    layoutResultsView()
    layoutHistoryAndCategoryTabView()
    layoutSearchNoResultsView()
    layoutSearchingView()
  }

  private func layoutResultsView() {
    containerView.addSubview(resultsTableView)
    resultsTableView.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      resultsTableView.topAnchor.constraint(equalTo: containerView.topAnchor),
      resultsTableView.leadingAnchor.constraint(equalTo: containerView.leadingAnchor),
      resultsTableView.trailingAnchor.constraint(equalTo: containerView.trailingAnchor),
      resultsTableView.bottomAnchor.constraint(equalTo: containerView.bottomAnchor)
    ])
  }

  private func layoutHistoryAndCategoryTabView() {
    containerView.addSubview(historyAndCategoryTabViewController.view)
    historyAndCategoryTabViewController.view.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      historyAndCategoryTabViewController.view.topAnchor.constraint(equalTo: containerView.topAnchor),
      historyAndCategoryTabViewController.view.leadingAnchor.constraint(equalTo: containerView.leadingAnchor),
      historyAndCategoryTabViewController.view.trailingAnchor.constraint(equalTo: containerView.trailingAnchor),
      historyAndCategoryTabViewController.view.bottomAnchor.constraint(equalTo: containerView.bottomAnchor)
    ])
  }

  private func layoutSearchNoResultsView() {
    searchNoResultsView.translatesAutoresizingMaskIntoConstraints = false
    containerView.addSubview(searchNoResultsView)
    NSLayoutConstraint.activate([
      searchNoResultsView.topAnchor.constraint(equalTo: containerView.topAnchor),
      searchNoResultsView.leadingAnchor.constraint(equalTo: containerView.leadingAnchor),
      searchNoResultsView.trailingAnchor.constraint(equalTo: containerView.trailingAnchor),
      searchNoResultsView.bottomAnchor.constraint(equalTo: containerView.bottomAnchor)
    ])
  }

  private func layoutSearchingView() {
    containerView.insertSubview(searchingActivityView, at: 0)
    searchingActivityView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      searchingActivityView.leadingAnchor.constraint(equalTo: containerView.leadingAnchor),
      searchingActivityView.trailingAnchor.constraint(equalTo: containerView.trailingAnchor),
      searchingActivityView.topAnchor.constraint(equalTo: containerView.topAnchor),
      searchingActivityView.bottomAnchor.constraint(equalTo: containerView.bottomAnchor)
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
    UIView.transition(with: containerView,
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
    interactor.handle(.didSelectResult(result, withSearchText: headerView.searchText))
    tableView.deselectRow(at: indexPath, animated: true)
  }

  func scrollViewWillBeginDragging(_ scrollView: UIScrollView) {
    interactor.handle(.didStartDraggingSearch)
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
    interactor.handle(.didStartTyping)
  }

  func searchBar(_ searchBar: UISearchBar, textDidChange searchText: String) {
    guard !searchText.isEmpty else {
      interactor.handle(.clearButtonDidTap)
      return
    }
    interactor.handle(.didType(SearchText(searchText, locale: searchBar.textInputMode?.primaryLanguage)))
  }

  func searchBarSearchButtonClicked(_ searchBar: UISearchBar) {
    guard let searchText = searchBar.text, !searchText.isEmpty else { return }
    interactor.handle(.searchButtonDidTap(SearchText(searchText, locale: searchBar.textInputMode?.primaryLanguage)))
  }

  func cancelButtonDidTap() {
    interactor.handle(.closeSearch)
  }
}

// MARK: - SearchTabViewControllerDelegate
extension SearchOnMapViewController: SearchTabViewControllerDelegate {
  func searchTabController(_ viewController: SearchTabViewController, didSearch text: String, withCategory: Bool) {
    interactor.handle(.didSelectText(SearchText(text, locale: nil), isCategory: withCategory))
  }
}

