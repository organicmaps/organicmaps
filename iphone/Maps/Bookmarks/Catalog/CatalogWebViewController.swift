@objc(MWMCatalogWebViewController)
final class CatalogWebViewController: WebViewController {

  let progressView = UIVisualEffectView(effect: UIBlurEffect(style: .dark))
  let progressImageView = UIImageView(image: #imageLiteral(resourceName: "ic_24px_spinner"))
  let numberOfTasksLabel = UILabel()
  let loadingIndicator = UIActivityIndicatorView(activityIndicatorStyle: .gray)
  var deeplink: URL?
  var statSent = false
  var backButton: UIBarButtonItem!
  var fwdButton: UIBarButtonItem!
  var toolbar = UIToolbar()

  @objc init() {
    super.init(url: MWMBookmarksManager.catalogFrontendUrl()!, title: L("guides"))!
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

  override func webView(_ webView: WKWebView,
                        decidePolicyFor navigationAction: WKNavigationAction,
                        decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
    guard let url = navigationAction.request.url, url.scheme == "mapsme" else {
      super.webView(webView, decidePolicyFor: navigationAction, decisionHandler: decisionHandler)
      return
    }

    processDeeplink(url)
    decisionHandler(.cancel);
  }

  override func webView(_ webView: WKWebView, didCommit navigation: WKNavigation!) {
    if !statSent {
      Statistics.logEvent("Bookmarks_Downloaded_Catalogue_open")
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

  func processDeeplink(_ url: URL) {
    let components = URLComponents(url: url, resolvingAgainstBaseURL: false)
    var id = ""
    var name = ""
    components?.queryItems?.forEach {
      if $0.name == "name" {
        name = $0.value ?? ""
      }
      if $0.name == "id" {
        id = $0.value ?? ""
      }
    }

    if MWMBookmarksManager.isCategoryDownloading(id) || MWMBookmarksManager.hasCategoryDownloaded(id) {
      MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: L("error_already_downloaded_guide"),
                                                               message: nil,
                                                               rightButtonTitle: L("ok"),
                                                               leftButtonTitle: nil,
                                                               rightButtonAction: nil)
      return
    }

    MWMBookmarksManager.downloadItem(withId: id, name: name, progress: { [weak self] (progress) in
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
          case .forbidden:
            fallthrough
          case .notFound:
            self?.showServerError(url)
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
        if MWMBookmarksManager.getCatalogDownloadsCount() == 0 {
          MapViewController.shared().showBookmarksLoadedAlert(categoryId)
        }
      }
      self?.updateProgress()
    }
  }

  private func showDiskError() {
    MWMAlertViewController.activeAlert().presentDownloaderNotEnoughSpaceAlert()
  }

  private func showNetworkError() {
    MWMAlertViewController.activeAlert().presentNoConnectionAlert();
  }

  private func showServerError(_ url: URL) {
    MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: L("error_server_title"),
                                                             message: L("error_server_message"),
                                                             rightButtonTitle: L("try_again"),
                                                             leftButtonTitle: L("cancel")) {
                                                              self.processDeeplink(url)
    }
  }

  private func showImportError() {
    MWMAlertViewController.activeAlert().presentInfoAlert(L("title_error_downloading_bookmarks"),
                                                          text: L("subtitle_error_downloading_guide"))
  }

  private func updateProgress() {
    let numberOfTasks = MWMBookmarksManager.getCatalogDownloadsCount()
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
