import UIKit

@objc final class NavigationSearch: NSObject {
  let searchController: UISearchController?
  let searchBar: UISearchBar

  private weak var viewController: UIViewController?
  private var rightBarButtonItems: [UIBarButtonItem]?
  private var showsSearchButton = true
  private var previousTitleView: UIView?

  private lazy var searchButton = UIBarButtonItem(
    barButtonSystemItem: .search,
    target: self,
    action: #selector(didTapSearch)
  )
  private lazy var closeButton = UIBarButtonItem(
    barButtonSystemItem: .close,
    target: self,
    action: #selector(didTapClose)
  )

  var isActive: Bool {
    searchController?.isActive ?? (viewController?.navigationItem.titleView === searchBar)
  }

  @objc(initWithViewController:delegate:placeholder:hidesNavigationBarDuringPresentation:hidesSearchBarWhenScrolling:)
  init(viewController: UIViewController,
       delegate: UISearchBarDelegate,
       placeholder: String,
       hidesNavigationBarDuringPresentation: Bool = false,
       hidesSearchBarWhenScrolling: Bool = false) {
    self.viewController = viewController

    // UIKit can assert when presenting UISearchController in an iPad app on older macOS (FB24201508).
    let processInfo = ProcessInfo.processInfo
    let macOSWithSearchFix = OperatingSystemVersion(majorVersion: 27, minorVersion: 2, patchVersion: 0)
    if processInfo.isiOSAppOnMac, !processInfo.isOperatingSystemAtLeast(macOSWithSearchFix) {
      searchController = nil
      searchBar = UISearchBar(frame: CGRect(x: 0, y: 0, width: 320, height: 44))
    } else {
      let controller = UISearchController(searchResultsController: nil)
      searchController = controller
      searchBar = controller.searchBar
    }
    super.init()

    searchBar.placeholder = placeholder
    searchBar.delegate = delegate
    if let searchController {
      searchController.obscuresBackgroundDuringPresentation = false
      searchController.hidesNavigationBarDuringPresentation = hidesNavigationBarDuringPresentation
      viewController.navigationItem.searchController = searchController
      viewController.navigationItem.hidesSearchBarWhenScrolling = hidesSearchBarWhenScrolling
    } else {
      rightBarButtonItems = viewController.navigationItem.rightBarButtonItems
      searchBar.searchBarStyle = .minimal
      searchBar.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
      searchButton.accessibilityLabel = placeholder
      updateInlineSearchButtons()
    }
    searchBar.applyTheme()
  }

  func setRightBarButtonItems(_ items: [UIBarButtonItem]?, showsSearchButton: Bool = true) {
    if searchController != nil {
      viewController?.navigationItem.rightBarButtonItems = items
      return
    }

    rightBarButtonItems = items
    self.showsSearchButton = showsSearchButton
    updateInlineSearchButtons()
  }

  func setShowsCancelButton(_ showsCancelButton: Bool) {
    searchController?.searchBar.setShowsCancelButton(showsCancelButton, animated: true)
  }

  func activate() {
    if let searchController {
      searchController.isActive = true
    } else if let navigationItem = viewController?.navigationItem, !isActive {
      previousTitleView = navigationItem.titleView
      navigationItem.titleView = searchBar
      updateInlineSearchButtons()
    }
  }

  func focusSearchBar() {
    if searchController == nil {
      viewController?.navigationController?.navigationBar.layoutIfNeeded()
      searchBar.searchTextField.becomeFirstResponder()
    } else {
      searchBar.becomeFirstResponder()
    }
  }

  func deactivate() {
    if let searchController {
      searchController.isActive = false
    } else if isActive, let navigationItem = viewController?.navigationItem {
      navigationItem.titleView = previousTitleView
      previousTitleView = nil
      updateInlineSearchButtons()
      searchBar.searchTextField.resignFirstResponder()
    }
  }

  private func updateInlineSearchButtons() {
    guard let navigationItem = viewController?.navigationItem else { return }

    var items = rightBarButtonItems ?? []
    if isActive {
      items.insert(closeButton, at: 0)
    } else if showsSearchButton {
      items.insert(searchButton, at: 0)
    }
    navigationItem.rightBarButtonItems = items
  }

  @objc private func didTapSearch() {
    activate()
    focusSearchBar()
  }

  @objc private func didTapClose() {
    deactivate()
    searchBar.delegate?.searchBarCancelButtonClicked?(searchBar)
  }
}
