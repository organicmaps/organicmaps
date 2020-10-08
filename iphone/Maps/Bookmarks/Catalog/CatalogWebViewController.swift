struct CatalogCategoryInfo {
  var id: String
  var name: String
  var author: String?
  var productId: String?
  var imageUrl: String?
  var subscriptionType: SubscriptionGroupType

  var hasSinglePurchase: Bool {
    return productId != nil
  }

  init?(_ components: [String: String], type: SubscriptionGroupType) {
    guard let id = components["id"],
      let name = components["name"] else { return nil }
    self.id = id
    self.name = name
    author = components["author_name"]
    productId = components["tier"]
    imageUrl = components["img"]
    subscriptionType = type
  }
}

@objc(MWMCatalogWebViewController)
final class CatalogWebViewController: WebViewController {
  let progressBgView = UIVisualEffectView(effect:
    UIBlurEffect(style: UIColor.isNightMode() ? .light : .dark))
  let progressView = ActivityIndicator()
  let numberOfTasksLabel = UILabel()
  let loadingIndicator = UIActivityIndicatorView(style: .gray)
  let pendingTransactionsHandler = InAppPurchase.pendingTransactionsHandler()
  var deeplink: URL?
  var categoryInfo: CatalogCategoryInfo?
  var statSent = false
  var billing = InAppPurchase.inAppBilling()
  var noInternetView: CatalogConnectionErrorView!

  @objc static func catalogFromAbsoluteUrl(_ url: URL? = nil, utm: MWMUTM = .none) -> CatalogWebViewController {
    return CatalogWebViewController(url, utm: utm, isAbsoluteUrl: true)
  }

  @objc static func catalogFromDeeplink(_ url: URL, utm: MWMUTM = .none) -> CatalogWebViewController {
    return CatalogWebViewController(url, utm: utm)
  }

  private init(_ url: URL? = nil, utm: MWMUTM = .none, isAbsoluteUrl: Bool = false) {
    var catalogUrl = BookmarksManager.shared().catalogFrontendUrl(utm)!
    if let u = url {
      if isAbsoluteUrl {
        catalogUrl = u
      } else {
        if u.host == "guides_page" {
          if let urlComponents = URLComponents(url: u, resolvingAgainstBaseURL: false),
            let path = urlComponents.queryItems?.reduce(into: "", { if $1.name == "url" { $0 = $1.value } }),
            let calculatedUrl = BookmarksManager.shared().catalogFrontendUrlPlusPath(path, utm: utm) {
            catalogUrl = calculatedUrl
          }
        } else {
          deeplink = url
        }
        Statistics.logEvent(kStatCatalogOpen, withParameters: [kStatFrom: kStatDeeplink])
      }
    }
    super.init(url: catalogUrl, title: L("guides_catalogue_title"))!
    noInternetView = CatalogConnectionErrorView(frame: .zero, actionCallback: { [weak self] in
      guard let self = self else { return }
      if !FrameworkHelper.isNetworkConnected() {
        self.noInternetView.isHidden = false
        return
      }

      self.noInternetView.isHidden = true
      self.loadingIndicator.startAnimating()
      if self.webView.url != nil {
        self.webView.reload()
      } else {
        self.performURLRequest()
      }
    })
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    numberOfTasksLabel.translatesAutoresizingMaskIntoConstraints = false
    progressView.translatesAutoresizingMaskIntoConstraints = false
    progressBgView.translatesAutoresizingMaskIntoConstraints = false
    loadingIndicator.translatesAutoresizingMaskIntoConstraints = false
    numberOfTasksLabel.styleName = "medium14:whiteText"
    numberOfTasksLabel.text = "0"
    progressBgView.layer.cornerRadius = 8
    progressBgView.clipsToBounds = true
    progressBgView.contentView.addSubview(progressView)
    progressBgView.contentView.addSubview(numberOfTasksLabel)
    view.addSubview(progressBgView)
    loadingIndicator.startAnimating()
    view.addSubview(loadingIndicator)

    loadingIndicator.centerXAnchor.constraint(equalTo: view.centerXAnchor).isActive = true
    loadingIndicator.centerYAnchor.constraint(equalTo: view.centerYAnchor).isActive = true
    numberOfTasksLabel.centerXAnchor.constraint(equalTo: progressBgView.centerXAnchor).isActive = true
    numberOfTasksLabel.centerYAnchor.constraint(equalTo: progressBgView.centerYAnchor).isActive = true
    progressView.centerXAnchor.constraint(equalTo: progressBgView.centerXAnchor).isActive = true
    progressView.centerYAnchor.constraint(equalTo: progressBgView.centerYAnchor).isActive = true
    progressBgView.widthAnchor.constraint(equalToConstant: 48).isActive = true
    progressBgView.heightAnchor.constraint(equalToConstant: 48).isActive = true

    let connected = FrameworkHelper.isNetworkConnected()
    if !connected {
      Statistics.logEvent("Bookmarks_Downloaded_Catalogue_error",
                          withParameters: [kStatError: "no_internet"])
    }

    noInternetView.isHidden = connected
    noInternetView.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(noInternetView)
    noInternetView.centerXAnchor.constraint(equalTo: view.centerXAnchor).isActive = true
    noInternetView.centerYAnchor.constraint(equalTo: view.centerYAnchor, constant: -20.0).isActive = true

    if #available(iOS 11, *) {
      progressBgView.leftAnchor.constraint(equalTo: view.safeAreaLayoutGuide.leftAnchor, constant: 8).isActive = true
    } else {
      progressBgView.leftAnchor.constraint(equalTo: view.leftAnchor, constant: 8).isActive = true
    }

    progressView.styleName = "MWMWhite"
    view.styleName = "Background"

    updateProgress()
    navigationItem.leftBarButtonItem = UIBarButtonItem(image: UIImage(named: "ic_nav_bar_back"),
                                                       style: .plain,
                                                       target: self,
                                                       action: #selector(onBackPressed))
    navigationItem.rightBarButtonItem = UIBarButtonItem(title: L("core_exit"),
                                                        style: .plain,
                                                        target: self,
                                                        action: #selector(onExitPressed))
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    if let deeplink = deeplink {
      processDeeplink(deeplink)
    }
  }

  override func willLoadUrl(_ decisionHandler: @escaping (Bool, [String: String]?) -> Void) {
    buildHeaders { [weak self] headers in
      self?.handlePendingTransactions {
        decisionHandler($0, headers)
        self?.checkInvalidSubscription()
      }
    }
  }

  override func shouldAddAccessToken() -> Bool {
    return true
  }

  override func webView(_ webView: WKWebView,
                        decidePolicyFor navigationAction: WKNavigationAction,
                        decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
    let subscribePath = "subscribe"
    let showOnMapPath = "map"
    guard let url = navigationAction.request.url,
      url.scheme == "mapsme" ||
        url.pathComponents.contains("buy_kml") ||
        url.pathComponents.contains(subscribePath) ||
        url.pathComponents.contains(showOnMapPath) else {
          super.webView(webView, decidePolicyFor: navigationAction, decisionHandler: decisionHandler)
          return
    }

    defer {
      decisionHandler(.cancel)
    }

    if url.pathComponents.contains(subscribePath) {
      showSubscriptionBannerScreen(SubscriptionGroupType(catalogURL: url))
      return
    }

    if url.pathComponents.contains(showOnMapPath) {
      guard let components = url.queryParams() else { return }
      guard let serverId = components["server_id"] else { return }
      showOnMap(serverId)
      return
    }

    processDeeplink(url)
  }

  override func webView(_ webView: WKWebView, didCommit navigation: WKNavigation!) {
    if !statSent {
      statSent = true
      MWMEye.boomarksCatalogShown()
    }
    loadingIndicator.stopAnimating()
  }

  override func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
    loadingIndicator.stopAnimating()
    Statistics.logEvent("Bookmarks_Downloaded_Catalogue_error",
                        withParameters: [kStatError: kStatUnknown])
  }

  override func webView(_ webView: WKWebView,
                        didFailProvisionalNavigation navigation: WKNavigation!,
                        withError error: Error) {
    loadingIndicator.stopAnimating()
    Statistics.logEvent("Bookmarks_Downloaded_Catalogue_error",
                        withParameters: [kStatError: kStatUnknown])
  }

  private func showOnMap(_ serverId: String) {
    let groupId = BookmarksManager.shared().getGroupId(serverId)
    FrameworkHelper.show(onMap: groupId)
    navigationController?.popToRootViewController(animated: true)
  }

  private func buildHeaders(completion: @escaping ([String: String]?) -> Void) {
    billing.requestProducts(Set(MWMPurchaseManager.bookmarkInappIds()), completion: { products, error in
      var productsInfo: [String: [String: String]] = [:]
      if let products = products {
        let formatter = NumberFormatter()
        formatter.numberStyle = .currency
        for product in products {
          formatter.locale = product.priceLocale
          let formattedPrice = formatter.string(from: product.price) ?? ""
          let pd: [String: String] = ["price_string": formattedPrice]
          productsInfo[product.productId] = pd
        }
      }
      guard let jsonData = try? JSONSerialization.data(withJSONObject: productsInfo, options: []),
        let jsonString = String(data: jsonData, encoding: .utf8),
        let encodedString = jsonString.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) else {
          completion(nil)
          return
      }

      var result = BookmarksManager.shared().getCatalogHeaders()
      result["X-Mapsme-Bundle-Tiers"] = encodedString
      completion(result)
    })
  }

  private func handlePendingTransactions(completion: @escaping (Bool) -> Void) {
    pendingTransactionsHandler.handlePendingTransactions { [weak self] status in
      switch status {
      case .none:
        fallthrough
      case .success:
        completion(true)
      case .error:
        MWMAlertViewController.activeAlert().presentInfoAlert(L("title_error_downloading_bookmarks"),
                                                              text: L("failed_purchase_support_message"))
        completion(false)
        self?.loadingIndicator.stopAnimating()
      case .needAuth:
        if let s = self, let navBar = s.navigationController?.navigationBar {
          s.signup(anchor: navBar, source: .guideCatalogue, onComplete: { result in
            if result == .succes {
              s.reloadFromOrigin()
              s.handlePendingTransactions(completion: completion)
            } else if result == .error {
              MWMAlertViewController.activeAlert().presentInfoAlert(L("title_error_downloading_bookmarks"),
                                                                    text: L("failed_purchase_support_message"))
              completion(false)
              s.loadingIndicator.stopAnimating()
            }
          })
        }
      }
    }
  }

  private func parseUrl(_ url: URL) -> CatalogCategoryInfo? {
    guard let components = url.queryParams() else { return nil }
    return CatalogCategoryInfo(components, type: SubscriptionGroupType(catalogURL: url))
  }

  func processDeeplink(_ url: URL) {
    deeplink = nil
    guard let categoryInfo = parseUrl(url) else {
      MWMAlertViewController.activeAlert().presentInfoAlert(L("title_error_downloading_bookmarks"),
                                                            text: L("subtitle_error_downloading_guide"))
      return
    }
    self.categoryInfo = categoryInfo

    download()
  }

  private func download() {
    guard let categoryInfo = self.categoryInfo else {
      assert(false)
      return
    }

    if BookmarksManager.shared().isCategoryDownloading(categoryInfo.id) || BookmarksManager.shared().hasCategoryDownloaded(categoryInfo.id) {
      MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: L("error_already_downloaded_guide"),
                                                               message: nil,
                                                               rightButtonTitle: L("ok"),
                                                               leftButtonTitle: nil,
                                                               rightButtonAction: nil)
      return
    }

    BookmarksManager.shared().downloadItem(withId: categoryInfo.id, name: categoryInfo.name, progress: { [weak self] progress in
      self?.updateProgress()
    }) { [weak self] categoryId, error in
      if let error = error as NSError? {
        if error.code == kCategoryDownloadFailedCode {
          guard let statusCode = error.userInfo[kCategoryDownloadStatusKey] as? NSNumber else {
            assertionFailure()
            return
          }
          guard let status = MWMCategoryDownloadStatus(rawValue: statusCode.intValue) else {
            assertionFailure()
            return
          }
          switch status {
          case .needAuth:
            if let s = self, let navBar = s.navigationController?.navigationBar {
              s.signup(anchor: navBar, source: .guideCatalogue) { result in
                if result == .succes {
                  s.reloadFromOrigin()
                  s.download()
                } else if result == .error {
                  MWMAlertViewController.activeAlert().presentAuthErrorAlert {
                    s.download()
                  }
                }
              }
            }
          case .needPayment:
            if categoryInfo.hasSinglePurchase {
              self?.showPaymentScreen(categoryInfo)
            } else {
              self?.showSubscriptionScreen(categoryInfo)
            }
          case .notFound:
            self?.showServerError()
          case .networkError:
            self?.showNetworkError()
          case .diskError:
            self?.showDiskError()
          }
        } else if error.code == kCategoryImportFailedCode {
          self?.showImportError()
        }
      } else {
        if BookmarksManager.shared().getCatalogDownloadsCount() == 0 {
          Statistics.logEvent(kStatInappProductDelivered, withParameters: [kStatVendor: BOOKMARKS_VENDOR,
                                                                           kStatPurchase: categoryInfo.id],
                              with: .realtime)
          logToPushWoosh(categoryInfo)
          MapViewController.shared().showBookmarksLoadedAlert(categoryId)
        }
      }
      self?.updateProgress()
    }
  }

  private func showPaymentScreen(_ productInfo: CatalogCategoryInfo) {
    guard let productId = productInfo.productId else {
      MWMAlertViewController.activeAlert().presentInfoAlert(L("title_error_downloading_bookmarks"),
                                                            text: L("subtitle_error_downloading_guide"))
      return
    }

    let purchase = InAppPurchase.paidRoutePurchase(serverId: productInfo.id,
                                                   productId: productId)
    let testGroup = ABTestManager.manager().paidRoutesSubscriptionCampaign.testGroupStatName
    let stats = InAppPurchase.paidRouteStatistics(serverId: productInfo.id,
                                                  productId: productId,
                                                  testGroup: testGroup,
                                                  source: kStatWebView)
    let paymentVC = PaidRouteViewController(name: productInfo.name,
                                            author: productInfo.author,
                                            imageUrl: URL(string: productInfo.imageUrl ?? ""),
                                            subscriptionType: productInfo.subscriptionType,
                                            purchase: purchase,
                                            statistics: stats)
    paymentVC.delegate = self
    paymentVC.modalTransitionStyle = .coverVertical
    paymentVC.modalPresentationStyle = .fullScreen
    navigationController?.present(paymentVC, animated: true)
  }

  private func showSubscriptionScreen(_ productInfo: CatalogCategoryInfo) {
    let subscribeViewController = SubscriptionViewBuilder.build(type: productInfo.subscriptionType,
                                                                parentViewController: self,
                                                                source: kStatWebView,
                                                                successDialog: .none) { [weak self] success in
                                                                  if success {
                                                                    self?.reloadFromOrigin()
                                                                    self?.download()
                                                                  }
    }
    present(subscribeViewController, animated: true)
  }

  private func showSubscriptionBannerScreen(_ type: SubscriptionGroupType) {
    let subscribeViewController = SubscriptionViewBuilder.build(type: type,
                                                                parentViewController: self,
                                                                source: kStatWebView,
                                                                successDialog: .success) { [weak self] success in
                                                                  if success {
                                                                    self?.reloadFromOrigin()
                                                                  }
    }
    present(subscribeViewController, animated: true)
  }

  private func showDiskError() {
    MWMAlertViewController.activeAlert().presentDownloaderNotEnoughSpaceAlert()
  }

  private func showNetworkError() {
    MWMAlertViewController.activeAlert().presentNoConnectionAlert()
  }

  private func showServerError() {
    MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: L("error_server_title"),
                                                             message: L("error_server_message"),
                                                             rightButtonTitle: L("try_again"),
                                                             leftButtonTitle: L("cancel")) {
                                                              self.download()
    }
  }

  private func showImportError() {
    MWMAlertViewController.activeAlert().presentInfoAlert(L("title_error_downloading_bookmarks"),
                                                          text: L("subtitle_error_downloading_guide"))
  }

  private func updateProgress() {
    let numberOfTasks = BookmarksManager.shared().getCatalogDownloadsCount()
    numberOfTasksLabel.text = "\(numberOfTasks)"
    progressBgView.isHidden = numberOfTasks == 0
  }

  @objc private func onBackPressed() {
    if webView.canGoBack {
      back()
      Statistics.logEvent(kStatGuidesBack, withParameters: [kStatMethod: kStatBack])
    } else {
      navigationController?.popViewController(animated: true)
      Statistics.logEvent(kStatGuidesClose, withParameters: [kStatMethod: kStatBack])
    }
  }

  @objc private func onExitPressed() {
    goBack()
    Statistics.logEvent(kStatGuidesClose, withParameters: [kStatMethod: kStatDone])
  }

  @objc private func onFwd() {
    forward()
  }
}

private func logToPushWoosh(_ categoryInfo: CatalogCategoryInfo) {
  let pushManager = PushNotificationManager.push()

  if categoryInfo.productId == nil {
    pushManager!.setTags(["Bookmarks_Guides_free_title": categoryInfo.name])
    pushManager!.setTags(["Bookmarks_Guides_free_date": MWMPushNotifications.formattedTimestamp()])
  } else {
    pushManager!.setTags(["Bookmarks_Guides_paid_tier": categoryInfo.productId!])
    pushManager!.setTags(["Bookmarks_Guides_paid_title": categoryInfo.name])
    pushManager!.setTags(["Bookmarks_Guides_paid_date": MWMPushNotifications.formattedTimestamp()])
  }
}

extension CatalogWebViewController: PaidRouteViewControllerDelegate {
  func didCompleteSubscription(_ viewController: PaidRouteViewController) {
    dismiss(animated: true)
    download()
    reloadFromOrigin()
  }

  func didCompletePurchase(_ viewController: PaidRouteViewController) {
    dismiss(animated: true)
    download()
    reloadFromOrigin()
  }

  func didCancelPurchase(_ viewController: PaidRouteViewController) {
    dismiss(animated: true)
  }
}
