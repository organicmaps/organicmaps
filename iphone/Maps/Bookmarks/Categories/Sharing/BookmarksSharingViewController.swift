import SafariServices

@objc
protocol BookmarksSharingViewControllerDelegate: AnyObject {
  func didShareCategory()
}

final class BookmarksSharingViewController: MWMTableViewController {
  typealias ViewModel = MWMAuthorizationViewModel
  
  @objc var categoryId = MWMFrameworkHelper.invalidCategoryId()
  var categoryUrl: URL?
  @objc weak var delegate: BookmarksSharingViewControllerDelegate?
  private var sharingTags: [MWMTag]?
  private var sharingUserStatus: MWMCategoryAuthorType?

  private var manager: MWMBookmarksManager {
    return MWMBookmarksManager.shared()
  }
  
  private var categoryAccessStatus: MWMCategoryAccessStatus? {
    guard categoryId != MWMFrameworkHelper.invalidCategoryId() else {
      assert(false)
      return nil
    }
    
    return manager.getCategoryAccessStatus(categoryId)
  }
  
  private let kPropertiesSegueIdentifier = "chooseProperties"
  private let kTagsControllerIdentifier = "tags"
  private let kEditOnWebSegueIdentifier = "editOnWeb"
  
  private let publicSectionIndex = 0
  private let privateSectionIndex = 1
  private let editOnWebCellIndex = 3
  private let rowsInPrivateSection = 2
  
  private var rowsInPublicSection: Int {
    return categoryAccessStatus == .public ? 4 : 3
  }
  
  @IBOutlet private weak var uploadAndPublishCell: UploadActionCell!
  @IBOutlet private weak var getDirectLinkCell: UploadActionCell!
  @IBOutlet private weak var editOnWebCell: UITableViewCell!
  
  @IBOutlet private weak var licenseAgreementTextView: UITextView! {
    didSet {
      let htmlString = String(coreFormat: L("ugc_routes_user_agreement"), arguments: [ViewModel.termsOfUseLink()])
      let attributes: [NSAttributedStringKey : Any] = [NSAttributedStringKey.font: UIFont.regular14(),
                                                       NSAttributedStringKey.foregroundColor: UIColor.blackSecondaryText()]
      licenseAgreementTextView.attributedText = NSAttributedString.string(withHtml: htmlString,
                                                                    defaultAttributes: attributes)
      licenseAgreementTextView.delegate = self
    }
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    title = L("sharing_options")
    configureActionCells()
    
    assert(categoryId != MWMFrameworkHelper.invalidCategoryId(), "We can't share nothing")

    guard let categoryAccessStatus = categoryAccessStatus else { return }

    switch categoryAccessStatus {
    case .local:
      break
    case .public:
      categoryUrl = manager.sharingUrl(forCategoryId: categoryId)
      uploadAndPublishCell.cellState = .completed
    case .private:
      categoryUrl = manager.sharingUrl(forCategoryId: categoryId)
      getDirectLinkCell.cellState = .completed
    case .other:
      break
    }
  }
  
  func configureActionCells() {
    uploadAndPublishCell.config(titles: [ .normal : L("upload_and_publish"),
                                          .inProgress : L("upload_and_publish_progress_text"),
                                          .completed : L("upload_and_publish_success") ],
                                image: UIImage(named: "ic24PxGlobe"),
                                delegate: self)
    getDirectLinkCell.config(titles: [ .normal : L("upload_and_get_direct_link"),
                                       .inProgress : L("direct_link_progress_text"),
                                       .completed : L("direct_link_success") ],
                             image: UIImage(named: "ic24PxLink"),
                             delegate: self)
  }
  
  override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    return UITableViewAutomaticDimension
  }
  
  override func numberOfSections(in _: UITableView) -> Int {
    return categoryAccessStatus == .public ? 1 : 2
  }
  
  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    switch section {
    case publicSectionIndex:
      return rowsInPublicSection
    case privateSectionIndex:
      return rowsInPrivateSection
    default:
      return 0
    }
  }
  
  override func tableView(_ tableView: UITableView,
                 titleForHeaderInSection section: Int) -> String? {
    return section == 0 ? L("public_access") : L("limited_access")
  }
  
  override func tableView(_ tableView: UITableView,
                 willSelectRowAt indexPath: IndexPath) -> IndexPath? {
    let cell = tableView.cellForRow(at: indexPath)
    if cell == getDirectLinkCell && getDirectLinkCell.cellState != .normal
      || cell == uploadAndPublishCell && uploadAndPublishCell.cellState != .normal {
      return nil
    }
    
    return indexPath
  }
  
  override func tableView(_ tableView: UITableView,
                          didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    
    let cell = tableView.cellForRow(at: indexPath)
    if cell == uploadAndPublishCell {
      startUploadAndPublishFlow()
    } else if cell == getDirectLinkCell {
      uploadAndGetDirectLink()
    } 
  }
  
  func startUploadAndPublishFlow() {
    Statistics.logEvent(kStatSharingOptionsClick, withParameters: [kStatItem : kStatPublic])
    performAfterValidation(anchor: uploadAndPublishCell) { [weak self] in
      if let self = self {
        self.performSegue(withIdentifier: self.kPropertiesSegueIdentifier, sender: self)
      }
    }
  }
  
  func uploadAndPublish() {
    guard categoryId != MWMFrameworkHelper.invalidCategoryId(),
      let tags = sharingTags,
      let userStatus = sharingUserStatus else {
        assert(false, "not enough data for public sharing")
        return
    }
    
    manager.setCategory(categoryId, authorType: userStatus)
    manager.setCategory(categoryId, tags: tags)
    manager.uploadAndPublishCategory(withId: categoryId, progress: { (progress) in
      self.uploadAndPublishCell.cellState = .inProgress
    }) { (url, error) in
      if let error = error as NSError? {
        self.uploadAndPublishCell.cellState = .normal
        self.showErrorAlert(error)
      } else {
        self.uploadAndPublishCell.cellState = .completed
        self.categoryUrl = url
        Statistics.logEvent(kStatSharingOptionsUploadSuccess, withParameters:
          [kStatTracks : self.manager.getCategoryTracksCount(self.categoryId),
           kStatPoints : self.manager.getCategoryMarksCount(self.categoryId)])
        
        self.tableView.beginUpdates()
        self.tableView.deleteSections(IndexSet(arrayLiteral: self.privateSectionIndex), with: .fade)
        self.tableView.insertRows(at: [IndexPath(item: self.editOnWebCellIndex,
                                                 section: self.publicSectionIndex)],
                                  with: .automatic)
        self.tableView.endUpdates()
      }
    }
  }
  
  func uploadAndGetDirectLink() {
    Statistics.logEvent(kStatSharingOptionsClick, withParameters: [kStatItem : kStatPrivate])
    performAfterValidation(anchor: getDirectLinkCell) { [weak self] in
      guard let s = self, s.categoryId != MWMFrameworkHelper.invalidCategoryId() else {
        assert(false, "categoryId must be valid")
        return
      }
      
      s.manager.uploadAndGetDirectLinkCategory(withId: s.categoryId, progress: { (progress) in
        if progress == .uploadStarted {
          s.getDirectLinkCell.cellState = .inProgress
        }
      }, completion: { (url, error) in
        if let error = error as NSError? {
          s.getDirectLinkCell.cellState = .normal
          s.showErrorAlert(error)
        } else {
          s.getDirectLinkCell.cellState = .completed
          s.categoryUrl = url
          s.delegate?.didShareCategory()
          Statistics.logEvent(kStatSharingOptionsUploadSuccess, withParameters:
            [kStatTracks : s.manager.getCategoryTracksCount(s.categoryId),
             kStatPoints : s.manager.getCategoryMarksCount(s.categoryId)])
        }
      })
    }
  }
  
  func performAfterValidation(anchor: UIView, action: @escaping MWMVoidBlock) {
    if MWMFrameworkHelper.isNetworkConnected() {
      signup(anchor: anchor, onComplete: { success in
        if success {
          action()
        } else {
          Statistics.logEvent(kStatSharingOptionsError, withParameters: [kStatError : 1])
        }
      })
    } else {
      Statistics.logEvent(kStatSharingOptionsError, withParameters: [kStatError : 0])
      MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: L("common_check_internet_connection_dialog_title"),
                                                               message: L("common_check_internet_connection_dialog"),
                                                               rightButtonTitle: L("downloader_retry"),
                                                               leftButtonTitle: L("cancel")) {
                                                                self.performAfterValidation(anchor: anchor,
                                                                                            action: action)
      }
    }
  }
  
  func showErrorAlert(_ error: NSError) {
    guard error.code == kCategoryUploadFailedCode,
      let statusCode = error.userInfo[kCategoryUploadStatusKey] as? Int,
      let status = MWMCategoryUploadStatus(rawValue: statusCode) else {
        assert(false)
        return
    }
    
    switch (status) {
    case .networkError:
      Statistics.logEvent(kStatSharingOptionsUploadError, withParameters: [kStatError : 1])
      self.showUploadError()
    case .serverError:
      Statistics.logEvent(kStatSharingOptionsUploadError, withParameters: [kStatError : 2])
      self.showUploadError()
    case .authError:
      Statistics.logEvent(kStatSharingOptionsUploadError, withParameters: [kStatError : 3])
      self.showUploadError()
    case .malformedData:
      Statistics.logEvent(kStatSharingOptionsUploadError, withParameters: [kStatError : 4])
      self.showMalformedDataError()
    case .accessError:
      Statistics.logEvent(kStatSharingOptionsUploadError, withParameters: [kStatError : 5])
      self.showAccessError()
    case .invalidCall:
      Statistics.logEvent(kStatSharingOptionsUploadError, withParameters: [kStatError : 6])
      assert(false, "sharing is not available for paid bookmarks")
    }
  }
  
  private func showUploadError() {
    MWMAlertViewController.activeAlert().presentInfoAlert(L("unable_upload_errorr_title"),
                                                          text: L("upload_error_toast"))
  }
  
  private func showMalformedDataError() {
    MWMAlertViewController.activeAlert().presentInfoAlert(L("unable_upload_errorr_title"),
                                                          text: L("unable_upload_error_subtitle_broken"))
  }
  
  private func showAccessError() {
    MWMAlertViewController.activeAlert().presentInfoAlert(L("unable_upload_errorr_title"),
                                                          text: L("unable_upload_error_subtitle_edited"))
  }
  
  override func prepare(for segue: UIStoryboardSegue, sender: Any?)  {
    if segue.identifier == kPropertiesSegueIdentifier {
      if let vc = segue.destination as? SharingPropertiesViewController {
        vc.delegate = self
      }
    } else if segue.identifier == kEditOnWebSegueIdentifier {
      Statistics.logEvent(kStatSharingOptionsClick, withParameters: [kStatItem : kStatEditOnWeb])
      if let vc = segue.destination as? EditOnWebViewController {
        vc.delegate = self
        vc.guideUrl = categoryUrl
      }
    }
  }
}

extension BookmarksSharingViewController: UITextViewDelegate {
  func textView(_ textView: UITextView, shouldInteractWith URL: URL, in characterRange: NSRange) -> Bool {
    let safari = SFSafariViewController(url: URL)
    present(safari, animated: true, completion: nil)
    return false
  }
}

extension BookmarksSharingViewController: UploadActionCellDelegate {
  func cellDidPressShareButton(_ cell: UploadActionCell, senderView: UIView) {
    guard let url = categoryUrl else {
      assert(false, "must provide guide url")
      return
    }
    
    Statistics.logEvent(kStatSharingOptionsClick, withParameters: [kStatItem : kStatCopyLink])
    let message = String(coreFormat: L("share_bookmarks_email_body_link"), arguments: [url.absoluteString])
    let shareController = MWMActivityViewController.share(for: nil, message: message) {
      _, success, _, _ in
      if success {
        Statistics.logEvent(kStatSharingLinkSuccess, withParameters: [kStatFrom : kStatSharingOptions])
      }
    }
    shareController?.present(inParentViewController: self, anchorView: senderView)
  }
}

extension BookmarksSharingViewController: SharingTagsViewControllerDelegate {
  func sharingTagsViewController(_ viewController: SharingTagsViewController, didSelect tags: [MWMTag]) {
    navigationController?.popViewController(animated: true)
    sharingTags = tags
    uploadAndPublish()
  }
  
  func sharingTagsViewControllerDidCancel(_ viewController: SharingTagsViewController) {
    navigationController?.popViewController(animated: true)
  }
}

extension BookmarksSharingViewController: SharingPropertiesViewControllerDelegate {
  func sharingPropertiesViewController(_ viewController: SharingPropertiesViewController,
                                       didSelect userStatus: MWMCategoryAuthorType) {
    sharingUserStatus = userStatus
    
    let storyboard = UIStoryboard.instance(.sharing)
    let tagsController = storyboard.instantiateViewController(withIdentifier: kTagsControllerIdentifier)
      as! SharingTagsViewController
    tagsController.delegate = self
    
    guard var viewControllers = navigationController?.viewControllers else {
      assert(false)
      return
    }
    
    viewControllers.removeLast()
    viewControllers.append(tagsController)
    navigationController?.setViewControllers(viewControllers, animated: true)
  }
}

extension BookmarksSharingViewController: EditOnWebViewControllerDelegate {
  func editOnWebViewControllerDidFinish(_ viewController: EditOnWebViewController) {
    dismiss(animated: true)
  }
}
