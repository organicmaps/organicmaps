protocol SearchOnMapView: AnyObject {
  func render(_ viewModel: SearchOnMap.ViewModel)
  func show()
  func close()
}

@objc
protocol SearchOnMapScrollViewDelegate: AnyObject {
  func scrollViewDidScroll(_ scrollView: UIScrollView)
  func scrollViewWillEndDragging(_ scrollView: UIScrollView, withVelocity velocity: CGPoint, targetContentOffset: UnsafeMutablePointer<CGPoint>)
}

@objc
protocol ModallyPresentedViewController: AnyObject {
  @objc func presentationFrameDidChange(_ frame: CGRect)
}

final class SearchOnMapViewController: UIViewController {
  typealias ViewModel = SearchOnMap.ViewModel
  typealias Content = SearchOnMap.ViewModel.Content

  fileprivate enum Constants {
    static let estimatedRowHeight: CGFloat = 80
    static let panGestureThreshold: CGFloat = 5
    static let dimAlpha: CGFloat = 0.3
    static let dimViewThreshold: CGFloat = 50
  }

  var interactor: SearchOnMapInteractor?

  @objc let availableAreaView = SearchOnMapAreaView()
  private let contentView = UIView()
  private let headerView = SearchOnMapHeaderView()
  private let searchResultsView = UIView()
  private let resultsTableView = UITableView()
  private let historyAndCategoryTabViewController = SearchTabViewController()
  private var searchingActivityView = PlaceholderView(hasActivityIndicator: true)
  private var searchNoResultsView = PlaceholderView(title: L("search_not_found"),
                                                    subtitle: L("search_not_found_query"))
  private var dimView: UIView?

  private var internalScrollViewContentOffset: CGFloat = .zero
  private let presentationStepsController = ModalPresentationStepsController()
  private var searchResults = SearchOnMap.SearchResults([])

  // MARK: - Init
  init() {
    super.init(nibName: nil, bundle: nil)
    configureModalPresentation()
  }

  private func configureModalPresentation() {
    guard let mapViewController = MapViewController.shared() else {
      fatalError("MapViewController is not available")
    }
    presentationStepsController.set(presentedView: availableAreaView, containerViewController: self)
    presentationStepsController.didUpdateHandler = presentationUpdateHandler

    mapViewController.searchContainer.addSubview(view)
    mapViewController.addChild(self)
    view.frame = mapViewController.searchContainer.bounds
    view.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    didMove(toParent: mapViewController)

    let affectedAreaViews = [
      mapViewController.sideButtonsArea,
      mapViewController.trafficButtonArea,
    ]
    affectedAreaViews.forEach { $0?.addAffectingView(availableAreaView) }
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  // MARK: - Lifecycle
  override func loadView() {
    view = TouchTransparentView()
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    setupViews()
    layoutViews()
    presentationStepsController.setInitialState()
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    headerView.setIsSearching(false)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateFrameOfPresentedViewInContainerView()
    updateDimView(for: availableAreaView.frame)
  }

  override func viewWillTransition(to size: CGSize, with coordinator: any UIViewControllerTransitionCoordinator) {
    super.viewWillTransition(to: size, with: coordinator)
    if #available(iOS 14.0, *), ProcessInfo.processInfo.isiOSAppOnMac {
      updateFrameOfPresentedViewInContainerView()
    }
  }

  // MARK: - Private methods
  private func setupViews() {
    availableAreaView.setStyleAndApply(.modalSheetBackground)
    contentView.setStyleAndApply(.modalSheetContent)

    setupGestureRecognizers()
    setupDimView()
    setupHeaderView()
    setupSearchResultsView()
    setupResultsTableView()
    setupHistoryAndCategoryTabView()
  }

  private func setupDimView() {
    iPhoneSpecific {
      dimView = UIView()
      dimView?.backgroundColor = .black
      dimView?.frame = view.bounds
    }
  }

  private func setupGestureRecognizers() {
    let tapGesture = UITapGestureRecognizer(target: self, action: #selector(handleTapOutside))
    tapGesture.cancelsTouchesInView = false
    contentView.addGestureRecognizer(tapGesture)

    iPhoneSpecific {
      let panGestureRecognizer = UIPanGestureRecognizer(target: self, action: #selector(handlePan(_:)))
      panGestureRecognizer.delegate = self
      contentView.addGestureRecognizer(panGestureRecognizer)
    }
  }

  private func setupHeaderView() {
    headerView.delegate = self
  }

  private func setupSearchResultsView() {
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
    if let dimView {
      view.addSubview(dimView)
      dimView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    }
    view.addSubview(availableAreaView)
    availableAreaView.addSubview(contentView)
    contentView.addSubview(searchResultsView)
    contentView.addSubview(headerView)

    contentView.translatesAutoresizingMaskIntoConstraints = false
    headerView.translatesAutoresizingMaskIntoConstraints = false
    searchResultsView.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      contentView.topAnchor.constraint(equalTo: availableAreaView.topAnchor),
      contentView.leadingAnchor.constraint(equalTo: availableAreaView.leadingAnchor),
      contentView.trailingAnchor.constraint(equalTo: availableAreaView.trailingAnchor),
      contentView.bottomAnchor.constraint(equalTo: availableAreaView.bottomAnchor),

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
    updateFrameOfPresentedViewInContainerView()
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

  // MARK: - Handle Presentation Steps
  private func updateFrameOfPresentedViewInContainerView() {
    presentationStepsController.updateMaxAvailableFrame()
    availableAreaView.frame = presentationStepsController.currentFrame
    view.layoutIfNeeded()
  }

  @objc
  private func handleTapOutside(_ gesture: UITapGestureRecognizer) {
    let location = gesture.location(in: view)
    if resultsTableView.frame.contains(location) && searchResults.isEmpty {
      headerView.setIsSearching(false)
    }
  }

  @objc
  private func handlePan(_ gesture: UIPanGestureRecognizer) {
    interactor?.handle(.didStartDraggingSearch)
    presentationStepsController.handlePan(gesture)
  }

  private var presentationUpdateHandler: (ModalPresentationStepsController.StepUpdate) -> Void {
    { [weak self] update in
      guard let self else { return }
      switch update {
      case .didClose:
        self.interactor?.handle(.closeSearch)
      case .didUpdateFrame(let frame):
        self.presentationFrameDidChange(frame)
        self.updateDimView(for: frame)
      case .didUpdateStep(let step):
        self.interactor?.handle(.didUpdatePresentationStep(step))
      }
    }
  }

  private func updateDimView(for frame: CGRect) {
    guard let dimView else { return }
    let currentTop = frame.origin.y
    let maxTop = presentationStepsController.maxAvailableFrame.origin.y
    let alpha = (1 - (currentTop - maxTop) / Constants.dimViewThreshold) * Constants.dimAlpha
    let isCloseToTop = currentTop - maxTop < Constants.dimViewThreshold
    let isPortrait = UIApplication.shared.statusBarOrientation.isPortrait
    let shouldDim = isCloseToTop && isPortrait
    UIView.animate(withDuration: kDefaultAnimationDuration / 2) {
      dimView.alpha = shouldDim ? alpha : 0
      dimView.isHidden = !shouldDim
    }
  }

  // MARK: - Handle Content Updates
  private func setContent(_ content: Content) {
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
    headerView.setSeparatorHidden(content == .historyAndCategory)
    showView(viewToShow(for: content))
  }

  private func viewToShow(for content: Content) -> UIView {
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
    UIView.animate(withDuration: kDefaultAnimationDuration / 2,
                   delay: 0,
                   options: .curveEaseInOut,
                   animations: {
      viewsToHide.forEach { $0.alpha = 0 }
      view.alpha = 1
    }) { _ in
      viewsToHide.forEach { $0.isHidden = true }
      view.isHidden = false
    }
  }

  private func setIsSearching(_ isSearching: Bool) {
    headerView.setIsSearching(isSearching)
  }

  private func setSearchText(_ text: String?) {
    if let text {
      headerView.setSearchText(text)
    }
  }
}

// MARK: - Public methods
extension SearchOnMapViewController: SearchOnMapView {
  func render(_ viewModel: ViewModel) {
    setContent(viewModel.contentState)
    setIsSearching(viewModel.isTyping)
    setSearchText(viewModel.searchingText)
    presentationStepsController.setStep(viewModel.presentationStep)
  }

  func show() {
    interactor?.handle(.openSearch)
  }

  func close() {
    headerView.setIsSearching(false)
    updateDimView(for: presentationStepsController.hiddenFrame)
    willMove(toParent: nil)
    presentationStepsController.close { [weak self] in
      self?.view.removeFromSuperview()
      self?.removeFromParent()
    }
  }
}

// MARK: - ModallyPresentedViewController
extension SearchOnMapViewController: ModallyPresentedViewController {
  func presentationFrameDidChange(_ frame: CGRect) {
    let translationY = frame.origin.y
    resultsTableView.contentInset.bottom = translationY
    historyAndCategoryTabViewController.presentationFrameDidChange(frame)
    searchNoResultsView.presentationFrameDidChange(frame)
    searchingActivityView.presentationFrameDidChange(frame)
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
    interactor?.handle(.didSelectResult(result, withQuery: headerView.searchQuery))
    tableView.deselectRow(at: indexPath, animated: true)
  }

  func scrollViewWillBeginDragging(_ scrollView: UIScrollView) {
    interactor?.handle(.didStartDraggingSearch)
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
    interactor?.handle(.didType(SearchQuery(searchText, locale: searchBar.textInputMode?.primaryLanguage, source: .typedText)))
  }

  func searchBarSearchButtonClicked(_ searchBar: UISearchBar) {
    guard let searchText = searchBar.text, !searchText.isEmpty else { return }
    interactor?.handle(.searchButtonDidTap(SearchQuery(searchText, locale: searchBar.textInputMode?.primaryLanguage, source: .typedText)))
  }

  func cancelButtonDidTap() {
    interactor?.handle(.closeSearch)
  }

  func grabberDidTap() {
    interactor?.handle(.didUpdatePresentationStep(.fullScreen))
  }
}

// MARK: - SearchTabViewControllerDelegate
extension SearchOnMapViewController: SearchTabViewControllerDelegate {
  func searchTabController(_ viewController: SearchTabViewController, didSearch query: SearchQuery) {
    interactor?.handle(.didSelect(query))
  }
}

// MARK: - UIGestureRecognizerDelegate
extension SearchOnMapViewController: UIGestureRecognizerDelegate {
  func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
    true
  }

  func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer) -> Bool {
    if gestureRecognizer is UIPanGestureRecognizer && otherGestureRecognizer is UIPanGestureRecognizer {
      // threshold is used to soften transition from the internal scroll zero content offset
      return internalScrollViewContentOffset < Constants.panGestureThreshold
    }
    return false
  }
}

// MARK: - SearchOnMapScrollViewDelegate
extension SearchOnMapViewController: SearchOnMapScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    let hasReachedTheTop = Int(availableAreaView.frame.origin.y) > Int(presentationStepsController.maxAvailableFrame.origin.y)
    let hasZeroContentOffset = internalScrollViewContentOffset == 0
    if hasReachedTheTop && hasZeroContentOffset {
      // prevent the internal scroll view scrolling
      scrollView.contentOffset.y = internalScrollViewContentOffset
      return
    }
    internalScrollViewContentOffset = scrollView.contentOffset.y
  }

  func scrollViewWillEndDragging(_ scrollView: UIScrollView, withVelocity velocity: CGPoint, targetContentOffset: UnsafeMutablePointer<CGPoint>) {
    // lock internal scroll view when the user fast scrolls screen to the top
    if internalScrollViewContentOffset == 0 {
      targetContentOffset.pointee = .zero
    }
  }
}
