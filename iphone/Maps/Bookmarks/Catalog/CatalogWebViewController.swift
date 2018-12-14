struct CatalogCategoryInfo {
  var id: String
  var name: String
  var author: String?
  var productId: String?
  var imageUrl: String?

  init?(_ components: [String : String]) {
    guard let id = components["id"],
      let name = components["name"] else { return nil }
    self.id = id
    self.name = name
    author = components["author_name"]
    productId = components["tier"]
    imageUrl = components["img"]
  }
}

@objc(MWMCatalogWebViewController)
final class CatalogWebViewController: WebViewController {
  let progressView = UIVisualEffectView(effect: UIBlurEffect(style: .dark))
  let progressImageView = UIImageView(image: #imageLiteral(resourceName: "ic_24px_spinner"))
  let numberOfTasksLabel = UILabel()
  let loadingIndicator = UIActivityIndicatorView(activityIndicatorStyle: .gray)
  let pendingTransactionsHandler = InAppPurchase.pendingTransactionsHandler()
  var deeplink: URL?
  var categoryInfo: CatalogCategoryInfo?
  var statSent = false
  var backButton: UIBarButtonItem!
  var fwdButton: UIBarButtonItem!
  var toolbar = UIToolbar()

  @objc init() {
    super.init(url: MWMBookmarksManager.shared().catalogFrontendUrl()!, title: L("guides"))!
    backButton = UIBarButtonItem(image: #imageLiteral(resourceName: "ic_catalog_back"), style: .plain, target: self, action: #selector(onBack))
    fwdButton = UIBarButtonItem(image: #imageLiteral(resourceName: "ic_catalog_fwd"), style: .plain, target: self, action: #selector(onFwd))
    backButton.tintColor = .blackSecondaryText()
    fwdButton.tintColor = .blackSecondaryText()
    backButton.isEnabled = false
    fwdButton.isEnabled = false
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  @objc convenience init(_ deeplinkURL: URL) {
    self.init()
    deeplink = deeplinkURL
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    numberOfTasksLabel.translatesAutoresizingMaskIntoConstraints = false
    progressImageView.translatesAutoresizingMaskIntoConstraints = false
    progressView.translatesAutoresizingMaskIntoConstraints = false
    loadingIndicator.translatesAutoresizingMaskIntoConstraints = false
    numberOfTasksLabel.font = UIFont.medium14()
    numberOfTasksLabel.textColor = UIColor.white
    numberOfTasksLabel.text = "0"
    progressView.layer.cornerRadius = 8
    progressView.clipsToBounds = true
    progressView.contentView.addSubview(progressImageView)
    progressView.contentView.addSubview(numberOfTasksLabel)
    view.addSubview(progressView)
    loadingIndicator.startAnimating()
    view.addSubview(loadingIndicator)

    loadingIndicator.centerXAnchor.constraint(equalTo: view.centerXAnchor).isActive = true
    loadingIndicator.centerYAnchor.constraint(equalTo: view.centerYAnchor).isActive = true
    numberOfTasksLabel.centerXAnchor.constraint(equalTo: progressView.centerXAnchor).isActive = true
    numberOfTasksLabel.centerYAnchor.constraint(equalTo: progressView.centerYAnchor).isActive = true
    progressImageView.centerXAnchor.constraint(equalTo: progressView.centerXAnchor).isActive = true
    progressImageView.centerYAnchor.constraint(equalTo: progressView.centerYAnchor).isActive = true
    progressView.widthAnchor.constraint(equalToConstant: 48).isActive = true
    progressView.heightAnchor.constraint(equalToConstant: 48).isActive = true

    view.addSubview(toolbar)
    toolbar.translatesAutoresizingMaskIntoConstraints = false
    toolbar.leftAnchor.constraint(equalTo: view.leftAnchor).isActive = true
    toolbar.rightAnchor.constraint(equalTo: view.rightAnchor).isActive = true
    toolbar.topAnchor.constraint(equalTo: progressView.bottomAnchor, constant: 8).isActive = true

    if #available(iOS 11, *) {
      toolbar.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor).isActive = true
      progressView.leftAnchor.constraint(equalTo: view.safeAreaLayoutGuide.leftAnchor, constant: 8).isActive = true
    } else {
      toolbar.bottomAnchor.constraint(equalTo: view.bottomAnchor).isActive = true
      progressView.leftAnchor.constraint(equalTo: view.leftAnchor, constant: 8).isActive = true
    }

    rotateProgress()
    updateProgress()
    navigationItem.leftBarButtonItem = UIBarButtonItem(image: #imageLiteral(resourceName: "ic_catalog_close"), style: .plain, target: self, action: #selector(goBack))
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    let fixedSpace = UIBarButtonItem(barButtonSystemItem: .fixedSpace, target: nil, action: nil)
    fixedSpace.width = 20
    toolbar.setItems([backButton, fixedSpace, fwdButton], animated: true)
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    if let deeplink = deeplink {
      processDeeplink(deeplink)
    }
  }

  override func willLoadUrl(_ decisionHandler: @escaping (Bool) -> Void) {
    handlePendingTransactions { decisionHandler($0) }
  }

  override func shouldAddAccessToken() -> Bool {
    return true
  }

  override func webView(_ webView: WKWebView,
                        decidePolicyFor navigationAction: WKNavigationAction,
                        decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
    guard let url = navigationAction.request.url,
      url.scheme == "mapsme" || url.path == "/mobilefront/buy_kml" else {
        super.webView(webView, decidePolicyFor: navigationAction, decisionHandler: decisionHandler)
        return
    }

    processDeeplink(url)
    decisionHandler(.cancel);
  }

  override func webView(_ webView: WKWebView, didCommit navigation: WKNavigation!) {
    if !statSent {
      statSent = true
      MWMEye.boomarksCatalogShown()
    }
    loadingIndicator.stopAnimating()
    backButton.isEnabled = webView.canGoBack
    fwdButton.isEnabled = webView.canGoForward
  }

  override func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
    Statistics.logEvent("Bookmarks_Downloaded_Catalogue_error",
                        withParameters: [kStatError : kStatUnknown])
    loadingIndicator.stopAnimating()
  }

  override func webView(_ webView: WKWebView,
                        didFailProvisionalNavigation navigation: WKNavigation!,
                        withError error: Error) {
    Statistics.logEvent("Bookmarks_Downloaded_Catalogue_error",
                        withParameters: [kStatError : kStatUnknown])
    loadingIndicator.stopAnimating()
  }

  private func handlePendingTransactions(completion: @escaping (Bool) -> Void) {
    pendingTransactionsHandler.handlePendingTransactions { [weak self] (status) in
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
        if let s = self {
          s.signup(anchor: s.toolbar, onComplete: {
            if $0 {
              s.handlePendingTransactions(completion: completion)
            } else {
              MWMAlertViewController.activeAlert().presentInfoAlert(L("title_error_downloading_bookmarks"),
                                                                    text: L("failed_purchase_support_message"))
              completion(false)
              s.loadingIndicator.stopAnimating()
            }
          })
        }
        break;
      }
    }
  }

  private func parseUrl(_ url: URL) -> CatalogCategoryInfo? {
    guard let urlComponents = URLComponents(url: url, resolvingAgainstBaseURL: false) else { return nil }
    guard let components = urlComponents.queryItems?.reduce(into: [:], { $0[$1.name] = $1.value })
      else { return nil }

    return CatalogCategoryInfo(components)
  }

  func processDeeplink(_ url: URL) {
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

    if MWMBookmarksManager.shared().isCategoryDownloading(categoryInfo.id) || MWMBookmarksManager.shared().hasCategoryDownloaded(categoryInfo.id) {
      MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: L("error_already_downloaded_guide"),
                                                               message: nil,
                                                               rightButtonTitle: L("ok"),
                                                               leftButtonTitle: nil,
                                                               rightButtonAction: nil)
      return
    }

    MWMBookmarksManager.shared().downloadItem(withId: categoryInfo.id, name: categoryInfo.name, progress: { [weak self] (progress) in
      self?.updateProgress()
    }) { [weak self] (categoryId, error) in
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
          switch (status) {
          case .needAuth:
            if let s = self {
              s.signup(anchor: s.toolbar) {
                if $0 { s.download() }
              }
            }
            break
          case .needPayment:
            self?.showPaymentScreen(categoryInfo)
            break
          case .notFound:
            self?.showServerError()
            break
          case .networkError:
            self?.showNetworkError()
            break
          case .diskError:
            self?.showDiskError()
            break
          }
        } else if error.code == kCategoryImportFailedCode {
          self?.showImportError()
        }
      } else {
        if MWMBookmarksManager.shared().getCatalogDownloadsCount() == 0 {
          Statistics.logEvent(kStatInappProductDelivered, withParameters: [kStatVendor: BOOKMARKS_VENDOR,
                                                                           kStatPurchase: categoryInfo.id])
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
    let stats = InAppPurchase.paidRouteStatistics(serverId: productInfo.id, productId: productId)
    let paymentVC = PaidRouteViewController(name: productInfo.name,
                                            author: productInfo.author,
                                            imageUrl: URL(string: productInfo.imageUrl ?? ""),
                                            purchase: purchase,
                                            statistics: stats)
    paymentVC.delegate = self
    paymentVC.modalTransitionStyle = .coverVertical
    self.navigationController?.present(paymentVC, animated: true)
  }

  private func showDiskError() {
    MWMAlertViewController.activeAlert().presentDownloaderNotEnoughSpaceAlert()
  }

  private func showNetworkError() {
    MWMAlertViewController.activeAlert().presentNoConnectionAlert();
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
    let numberOfTasks = MWMBookmarksManager.shared().getCatalogDownloadsCount()
    numberOfTasksLabel.text = "\(numberOfTasks)"
    progressView.isHidden = numberOfTasks == 0
  }

  private func rotateProgress() {
    let rotationAnimation = CABasicAnimation(keyPath: "transform.rotation.z")
    rotationAnimation.toValue = Double.pi * 2;
    rotationAnimation.duration = 1
    rotationAnimation.isCumulative = true
    rotationAnimation.repeatCount = Float(Int.max)

    progressImageView.layer.add(rotationAnimation, forKey:"rotationAnimation");
  }

  @objc private func onBack() {
    back()
  }

  @objc private func onFwd() {
    forward()
  }
}

extension CatalogWebViewController: PaidRouteViewControllerDelegate {
  func didCompletePurchase(_ viewController: PaidRouteViewController) {
    dismiss(animated: true)
    download()
  }

  func didCancelPurchase(_ viewController: PaidRouteViewController) {
    dismiss(animated: true)
  }
}
