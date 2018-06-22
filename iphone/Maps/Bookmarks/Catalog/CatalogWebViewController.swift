class CatalogWebViewController: WebViewController {

  let progressView = UIVisualEffectView(effect: UIBlurEffect(style: .dark))
  let progressImageView = UIImageView(image: #imageLiteral(resourceName: "ic_24px_spinner"))
  let numberOfTasksLabel = UILabel()
  
  override func viewDidLoad() {
    super.viewDidLoad()

    numberOfTasksLabel.translatesAutoresizingMaskIntoConstraints = false
    progressImageView.translatesAutoresizingMaskIntoConstraints = false
    progressView.translatesAutoresizingMaskIntoConstraints = false
    numberOfTasksLabel.font = UIFont.medium14()
    numberOfTasksLabel.textColor = UIColor.white
    numberOfTasksLabel.text = "0"
    progressView.layer.cornerRadius = 8
    progressView.clipsToBounds = true
    progressView.contentView.addSubview(progressImageView)
    progressView.contentView.addSubview(numberOfTasksLabel)
    view.addSubview(progressView)

    progressView.contentView.addConstraint(NSLayoutConstraint(item: numberOfTasksLabel,
                                                              attribute: .centerX,
                                                              relatedBy: .equal,
                                                              toItem: progressView.contentView,
                                                              attribute: .centerX,
                                                              multiplier: 1,
                                                              constant: 0))
    progressView.contentView.addConstraint(NSLayoutConstraint(item: numberOfTasksLabel,
                                                              attribute: .centerY,
                                                              relatedBy: .equal,
                                                              toItem: progressView.contentView,
                                                              attribute: .centerY,
                                                              multiplier: 1,
                                                              constant: 0))
    progressView.contentView.addConstraint(NSLayoutConstraint(item: progressImageView,
                                                              attribute: .centerX,
                                                              relatedBy: .equal,
                                                              toItem: progressView.contentView,
                                                              attribute: .centerX,
                                                              multiplier: 1,
                                                              constant: 0))
    progressView.contentView.addConstraint(NSLayoutConstraint(item: progressImageView,
                                                              attribute: .centerY,
                                                              relatedBy: .equal,
                                                              toItem: progressView.contentView,
                                                              attribute: .centerY,
                                                              multiplier: 1,
                                                              constant: 0))
    let views = ["pv": progressView]
    view.addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "H:|-8-[pv(48)]",
                                                       options: .directionLeftToRight,
                                                       metrics: [:],
                                                       views: views))
    view.addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "V:[pv(48)]-8-|",
                                                       options: .directionLeftToRight,
                                                       metrics: [:],
                                                       views: views))
    rotateProgress()
    updateProgress()
  }

  override func webView(_ webView: WKWebView,
                        decidePolicyFor navigationAction: WKNavigationAction,
                        decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
    if let url = navigationAction.request.url {
      if url.scheme == "mapsme" {
        processDeeplink(url)
        decisionHandler(.cancel)
        return
      }
    }
    super.webView(webView, decidePolicyFor: navigationAction, decisionHandler: decisionHandler)
  }

  func processDeeplink(_ url: URL) {
    let components = URLComponents(url: url, resolvingAgainstBaseURL: false)
    var id = ""
    var name = ""
    components?.queryItems?.forEach({ (item) in
      if (item.name == "name") {
        name = item.value ?? ""
      }
      if (item.name == "id") {
        id = item.value ?? ""
      }
    })

    MWMBookmarksManager.downloadItem(withId: id, name: name, progress: { (progress) in
      self.updateProgress()
    }) { (error) in
      if let error = error as NSError? {
        if error.code == kCategoryDownloadFailedCode {
          guard let statusCode = error.userInfo[kCategoryDownloadStatusKey] as? NSNumber else { return }
          guard let status = MWMCategoryDownloadStatus(rawValue: statusCode.intValue) else { return }
          switch (status) {
          case .forbidden:
            fallthrough
          case .notFound:
            self.showServerError(url)
            break
          case .networkError:
            self.showNetworkError()
            break
          case .diskError:
            self.showDiskError()
            break
          }
        } else if error.code == kCategoryImportFailedCode {
          self.showImportError()
        }
      } else {
        Toast.toast(withText: L("bookmarks_webview_success_toast")).show()
      }
    }
    self.updateProgress()
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
    // TODO: add import error alert
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
}
