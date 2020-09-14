protocol EditOnWebViewControllerDelegate: AnyObject {
  func editOnWebViewControllerDidFinish(_ viewController: EditOnWebViewController)
}

final class EditOnWebViewController: MWMViewController {
  weak var delegate: EditOnWebViewControllerDelegate?
  var category: BookmarkGroup!
  
  @IBOutlet weak var activityIndicator: ActivityIndicator!
  @IBOutlet weak var sendMeLinkButton: MWMButton! {
    didSet {
      sendMeLinkButton.setTitle(L("send_a_link_btn").uppercased(), for: .normal)
    }
  }
  
  @IBOutlet weak var cancelButton: UIButton! {
    didSet {
      cancelButton.setTitle(L("cancel").uppercased(), for: .normal)
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    assert(category != nil, "Category must be set")
  }

  override var hasNavigationBar: Bool {
    return false
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    return UIColor.isNightMode() ? .lightContent : .default
  }

  @IBAction func sendMeLinkButtonPressed(_ sender: Any) {
    Statistics.logEvent(kStatEditOnWebClick, withParameters: [kStatItem : kStatCopyLink])
    switch category.accessStatus {
    case .local:
      uploadCategory()
    case .public:
      fallthrough
    case .private:
      fallthrough
    case .authorOnly:
      presentSharingOptions()
    case .other:
      assert(false, "Unexpected")
    }
  }
  
  @IBAction func cancelButtonPressed(_ sender: Any) {
    delegate?.editOnWebViewControllerDidFinish(self)
  }

  private func uploadCategory() {
    activityIndicator.isHidden = false
    sendMeLinkButton.isEnabled = false
    sendMeLinkButton.setTitle(nil, for: .normal)
    BookmarksManager.shared().uploadCategory(withId: category.categoryId, progress: { (progress) in

    }) { [weak self] (error) in
      guard let self = self else { return }
      self.activityIndicator.isHidden = true
      self.sendMeLinkButton.isEnabled = true
      self.sendMeLinkButton.setTitle(L("send_a_link_btn").uppercased(), for: .normal)
      if let error = error as NSError? {
        guard error.code == kCategoryUploadFailedCode,
          let statusCode = error.userInfo[kCategoryUploadStatusKey] as? Int,
          let status = MWMCategoryUploadStatus(rawValue: statusCode) else {
            assert(false)
            return
        }

        switch status {
        case .networkError:
          self.networkError()
        case .authError:
          self.authError()
        case .malformedData:
          self.fileError()
        case .serverError:
          self.serverError()
        case .accessError:
          fallthrough
        case .invalidCall:
          self.anotherError()
        }
      } else {
        self.presentSharingOptions()
      }
    }
  }

  private func presentSharingOptions() {
    guard let url = BookmarksManager.shared().webEditorUrl(forCategoryId: category.categoryId,
                                                           language: AppInfo.shared().twoLetterLanguageId ) else {
      assert(false, "Unexpected empty url for category \(category.title)")
      return
    }

    let message = String(coreFormat: L("share_bookmarks_email_body_link"),
                         arguments: [url.absoluteString])
    let shareController = ActivityViewController.share(for: nil, message: message) {
      [weak self] _, success, _, _ in
      if success {
        Statistics.logEvent(kStatSharingLinkSuccess, withParameters: [kStatFrom : kStatEditOnWeb])
        if let self = self {
          self.delegate?.editOnWebViewControllerDidFinish(self)
        }
      }
    }
    shareController?.present(inParentViewController: self, anchorView: sendMeLinkButton)
  }

  private func networkError() {
    MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: L("common_check_internet_connection_dialog_title"),
                                                             message: L("common_check_internet_connection_dialog"),
                                                             rightButtonTitle: L("try_again"),
                                                             leftButtonTitle: L("cancel")) {
                                                              self.uploadCategory()
    }
  }

  private func serverError() {
    MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: L("error_server_title"),
                                                             message: L("error_server_message"),
                                                             rightButtonTitle: L("try_again"),
                                                             leftButtonTitle: L("cancel")) {
                                                              self.uploadCategory()
    }
  }

  private func authError() {
    signup(anchor: sendMeLinkButton, source: .exportBookmarks) {
      if ($0) {
        self.uploadCategory()
      }
    }
  }

  private func fileError() {
    MWMAlertViewController.activeAlert().presentInfoAlert(L("unable_upload_errorr_title"),
                                                          text: L("unable_upload_error_subtitle_broken"))
  }

  private func anotherError() {
    MWMAlertViewController.activeAlert().presentInfoAlert(L("unable_upload_errorr_title"),
                                                          text: L("upload_error_toast"))
  }
}
