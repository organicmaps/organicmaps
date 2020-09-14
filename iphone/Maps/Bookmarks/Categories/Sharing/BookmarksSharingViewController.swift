import SafariServices

@objc
protocol BookmarksSharingViewControllerDelegate: AnyObject {
  func didShareCategory()
}

final class BookmarksSharingViewController: MWMTableViewController {
  @objc var category: BookmarkGroup!
  @objc weak var delegate: BookmarksSharingViewControllerDelegate?
  private var sharingTags: [MWMTag]?
  private var sharingUserStatus: BookmarkGroupAuthorType?

  private let manager = BookmarksManager.shared()
  
  private let kPropertiesControllerIdentifier = "chooseProperties"
  private let kTagsControllerIdentifier = "tags"
  private let kNameControllerIdentifier = "guideName"
  private let kDescriptionControllerIdentifier = "guideDescription"
  private let kEditOnWebSegueIdentifier = "editOnWeb"
  
  private let privateSectionIndex = 0
  private let publicSectionIndex = 1
  private let editOnWebSectionIndex = 2
  private let rowsInEditOnWebSection = 1
  private let directLinkUpdateRowIndex = 2
  private let publishUpdateRowIndex = 2

  private var rowsInPublicSection: Int {
    return (category.accessStatus == .public && uploadAndPublishCell.cellState != .updating) ? 3 : 2
  }

  private var rowsInPrivateSection: Int {
    return (category.accessStatus == .private && getDirectLinkCell.cellState != .updating) ? 3 : 2
  }
  
  @IBOutlet weak var uploadAndPublishCell: UploadActionCell!
  @IBOutlet weak var getDirectLinkCell: UploadActionCell!
  @IBOutlet weak var updatePublishCell: UITableViewCell!
  @IBOutlet weak var updateDirectLinkCell: UITableViewCell!
  @IBOutlet weak var directLinkInstructionsLabel: UILabel!
  @IBOutlet weak var editOnWebButton: UIButton! {
    didSet {
      editOnWebButton.setTitle(L("edit_on_web").uppercased(), for: .normal)
    }
  }

  @IBOutlet private weak var licenseAgreementTextView: UITextView! {
    didSet {
      let htmlString = String(coreFormat: L("ugc_routes_user_agreement"),
                              arguments: [User.termsOfUseLink()])
      licenseAgreementTextView.attributedText = NSAttributedString.string(withHtml: htmlString,
                                                                          defaultAttributes: [:])
      licenseAgreementTextView.delegate = self
    }
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()

    assert(category != nil, "We can't share nothing")

    title = L("sharing_options")
    configureActionCells()

    switch category.accessStatus {
    case .local:
      break
    case .public:
      getDirectLinkCell.cellState = .disabled
      uploadAndPublishCell.cellState = .completed
      directLinkInstructionsLabel.text = L("unable_get_direct_link_desc")
    case .private:
      getDirectLinkCell.cellState = .completed
    case .authorOnly:
      break
    case .other:
      break
    }
  }
  
  private func configureActionCells() {
    uploadAndPublishCell.config(titles: [ .normal : L("upload_and_publish"),
                                          .inProgress : L("upload_and_publish_progress_text"),
                                          .updating : L("direct_link_updating_text"),
                                          .completed : L("upload_and_publish_success") ],
                                image: UIImage(named: "ic24PxGlobe"),
                                delegate: self)
    getDirectLinkCell.config(titles: [ .normal : L("upload_and_get_direct_link"),
                                       .inProgress : L("direct_link_progress_text"),
                                       .updating : L("direct_link_updating_text"),
                                       .completed : L("direct_link_success"),
                                       .disabled : L("upload_and_publish_success")],
                             image: UIImage(named: "ic24PxLink"),
                             delegate: self)
  }
  
  override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    return UITableView.automaticDimension
  }
  
  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    switch section {
    case publicSectionIndex:
      return rowsInPublicSection
    case privateSectionIndex:
      return rowsInPrivateSection
    case editOnWebSectionIndex:
      return rowsInEditOnWebSection
    default:
      return 0
    }
  }
  
  override func tableView(_ tableView: UITableView,
                 titleForHeaderInSection section: Int) -> String? {
    switch section {
    case 0:
      return L("limited_access")
    case 1:
      return L("public_access")
    case 2:
      return L("edit_on_web")
    default:
      return nil
    }
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
      uploadAndGetDirectLink(update: false)
    } else if cell == updatePublishCell {
      updatePublic()
    } else if cell == updateDirectLinkCell {
      updateDirectLink()
    }
  }

  private func updatePublic() {
    let updateAction = { [unowned self] in
      self.performAfterValidation(anchor: self.uploadAndPublishCell) { [weak self] in
        guard let self = self else { return }
        if self.category.title.isEmpty || self.category.detailedAnnotation.isEmpty {
          self.showMalformedDataError()
          return
        }
        if (self.category.trackCount + self.category.bookmarksCount < 3) {
          MWMAlertViewController
            .activeAlert()
            .presentInfoAlert(L("error_public_not_enought_title"),
                              text: L("error_public_not_enought_subtitle"))
          return
        }
        self.uploadAndPublish(update: true)
      }
    }
    MWMAlertViewController
      .activeAlert()
      .presentDefaultAlert(withTitle: L("any_access_update_alert_title"),
                           message: L("any_access_update_alert_message"),
                           rightButtonTitle: L("any_access_update_alert_update"),
                           leftButtonTitle: L("cancel"),
                           rightButtonAction: updateAction)
  }

  private func updateDirectLink() {
    MWMAlertViewController.activeAlert().presentDefaultAlert(withTitle: L("any_access_update_alert_title"),
                                                             message: L("any_access_update_alert_message"),
                                                             rightButtonTitle: L("any_access_update_alert_update"),
                                                             leftButtonTitle: L("cancel")) {
                                                              self.uploadAndGetDirectLink(update: true)
    }
  }

  private func startUploadAndPublishFlow() {
    Statistics.logEvent(kStatSharingOptionsClick, withParameters: [kStatItem : kStatPublic])
    let alert = EditOnWebAlertViewController(with: L("bookmark_public_upload_alert_title"),
                                             message: L("bookmark_public_upload_alert_subtitle"),
                                             acceptButtonTitle: L("bookmark_public_upload_alert_ok_button"))
    alert.onAcceptBlock = { [unowned self] in
      self.dismiss(animated: true)
      self.performAfterValidation(anchor: self.uploadAndPublishCell) { [weak self] in
        if let self = self {
          if (self.category.trackCount + self.category.bookmarksCount > 2) {
            self.showEditName()
          } else {
            MWMAlertViewController.activeAlert().presentInfoAlert(L("error_public_not_enought_title"),
                                                                  text: L("error_public_not_enought_subtitle"))
          }
        }
      }
    }
    alert.onCancelBlock = { [unowned self] in self.dismiss(animated: true) }
    present(alert, animated: true)
  }
  
  private func uploadAndPublish(update: Bool) {
    if !update {
      guard let tags = sharingTags, let userStatus = sharingUserStatus else {
        assert(false, "not enough data for public sharing")
        return
      }

      manager.setCategory(category.categoryId, authorType: userStatus)
      manager.setCategory(category.categoryId, tags: tags)
    }

    uploadAndPublishCell.cellState = update ? .updating : .inProgress
    if update {
      self.tableView.deleteRows(at: [IndexPath(row: self.publishUpdateRowIndex,
                                               section: self.publicSectionIndex)],
                                with: .automatic)
    }

    manager.uploadAndPublishCategory(withId: category.categoryId, progress: nil) { (error) in
      if let error = error as NSError? {
        self.uploadAndPublishCell.cellState = .normal
        self.showErrorAlert(error)
      } else {
        self.uploadAndPublishCell.cellState = .completed
        Statistics.logEvent(kStatSharingOptionsUploadSuccess, withParameters:
          [kStatTracks : self.category.trackCount,
           kStatPoints : self.category.bookmarksCount])

        self.getDirectLinkCell.cellState = .disabled
        self.directLinkInstructionsLabel.text = L("unable_get_direct_link_desc")
        self.tableView.beginUpdates()
        let directLinkUpdateIndexPath = IndexPath(row: self.directLinkUpdateRowIndex,
                                                  section: self.privateSectionIndex)
        if (self.tableView.cellForRow(at: directLinkUpdateIndexPath) != nil) {
          self.tableView.deleteRows(at: [directLinkUpdateIndexPath], with: .automatic)
        }

        self.tableView.insertRows(at: [IndexPath(row: self.publishUpdateRowIndex,
                                                 section: self.publicSectionIndex)],
                                  with: .automatic)
        self.tableView.endUpdates()
        if update {
          Toast.toast(withText: L("direct_link_updating_success")).show()
        }
      }
    }
  }
  
  private func uploadAndGetDirectLink(update: Bool) {
    Statistics.logEvent(kStatSharingOptionsClick, withParameters: [kStatItem : kStatPrivate])
    performAfterValidation(anchor: getDirectLinkCell) { [weak self] in
      guard let s = self else {
        assert(false, "Unexpected self == nil")
        return
      }
      
      s.getDirectLinkCell.cellState = update ? .updating : .inProgress
      if update {
        s.tableView.deleteRows(at: [IndexPath(item: s.directLinkUpdateRowIndex,
                                              section: s.privateSectionIndex)],
                               with: .automatic)
      }
      s.manager.uploadAndGetDirectLinkCategory(withId: s.category.categoryId, progress: nil) { (error) in
        if let error = error as NSError? {
          s.getDirectLinkCell.cellState = .normal
          s.showErrorAlert(error)
        } else {
          s.getDirectLinkCell.cellState = .completed
          s.delegate?.didShareCategory()
          Statistics.logEvent(kStatSharingOptionsUploadSuccess, withParameters:
            [kStatTracks : s.category.trackCount,
             kStatPoints : s.category.bookmarksCount])
          s.tableView.insertRows(at: [IndexPath(item: s.directLinkUpdateRowIndex,
                                                section: s.privateSectionIndex)],
                                 with: .automatic)
          if update {
            Toast.toast(withText: L("direct_link_updating_success")).show()
          }
        }
      }
    }
  }
  
  private func performAfterValidation(anchor: UIView, action: @escaping MWMVoidBlock) {
    if FrameworkHelper.isNetworkConnected() {
      signup(anchor: anchor, source: .exportBookmarks, onComplete: { success in
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
  
  private func showErrorAlert(_ error: NSError) {
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
    let alert = EditOnWebAlertViewController(with: L("html_format_error_title"),
                                             message: L("html_format_error_subtitle"),
                                             acceptButtonTitle: L("edit_on_web").uppercased())
    alert.onAcceptBlock = {
      self.dismiss(animated: true, completion: {
        self.performSegue(withIdentifier: self.kEditOnWebSegueIdentifier, sender: nil)
      })
    }
    alert.onCancelBlock = {
      self.dismiss(animated: true)
    }

    navigationController?.present(alert, animated: true)
  }
  
  private func showAccessError() {
    let alert = EditOnWebAlertViewController(with: L("public_or_limited_access_after_edit_online_error_title"),
                                             message: L("public_or_limited_access_after_edit_online_error_message"),
                                             acceptButtonTitle: L("edit_on_web").uppercased())
    alert.onAcceptBlock = {
      self.dismiss(animated: true, completion: {
        self.performSegue(withIdentifier: self.kEditOnWebSegueIdentifier, sender: nil)
      })
    }

    alert.onCancelBlock = {
      self.dismiss(animated: true)
    }

    navigationController?.present(alert, animated: true)
  }
  
  override func prepare(for segue: UIStoryboardSegue, sender: Any?)  {
    if segue.identifier == kEditOnWebSegueIdentifier {
      Statistics.logEvent(kStatSharingOptionsClick, withParameters: [kStatItem : kStatEditOnWeb])
      if let vc = segue.destination as? EditOnWebViewController {
        vc.delegate = self
        vc.category = category
      }
    }
  }

  private func showEditName() {
    let storyboard = UIStoryboard.instance(.sharing)
    let guideNameController = storyboard.instantiateViewController(withIdentifier: kNameControllerIdentifier)
      as! GuideSharingNameViewController
    guideNameController.guideName = category.title
    guideNameController.delegate = self
    navigationController?.pushViewController(guideNameController, animated: true)
  }

  private func showEditDescr() {
    let storyboard = UIStoryboard.instance(.sharing)
    let guideDescrController = storyboard.instantiateViewController(withIdentifier: kDescriptionControllerIdentifier)
      as! GuideSharingDescriptionViewController
    guideDescrController.guideDescription = category.detailedAnnotation
    guideDescrController.delegate = self

    replaceTopViewController(guideDescrController, animated: true)
  }

  private func showSelectTags() {
    let storyboard = UIStoryboard.instance(.sharing)
    let tagsController = storyboard.instantiateViewController(withIdentifier: kTagsControllerIdentifier)
      as! SharingTagsViewController
    tagsController.delegate = self

    replaceTopViewController(tagsController, animated: true)
  }

  private func showSelectProperties() {
    let storyboard = UIStoryboard.instance(.sharing)
    let propertiesController = storyboard.instantiateViewController(withIdentifier: kPropertiesControllerIdentifier)
      as! SharingPropertiesViewController
    propertiesController.delegate = self
    replaceTopViewController(propertiesController, animated: true)
  }

  private func replaceTopViewController(_ viewController: UIViewController, animated: Bool) {
    guard var viewControllers = navigationController?.viewControllers else {
      assert(false)
      return
    }

    viewControllers.removeLast()
    viewControllers.append(viewController)
    navigationController?.setViewControllers(viewControllers, animated: animated)
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
    if cell == uploadAndPublishCell {
      share(manager.publicLink(forCategoryId: category.categoryId), senderView: senderView)
    } else if cell == getDirectLinkCell {
      share(manager.deeplink(forCategoryId: category.categoryId), senderView: senderView)
    } else {
      assert(false, "unsupported cell")
    }
  }
  
  func share(_ url: URL?, senderView: UIView) {
    guard let url = url else {
      assert(false, "must provide guide url")
      return
    }
    
    Statistics.logEvent(kStatSharingOptionsClick, withParameters: [kStatItem : kStatCopyLink])
    let message = String(coreFormat: L("share_bookmarks_email_body_link"), arguments: [url.absoluteString])
    let shareController = ActivityViewController.share(for: nil, message: message) {
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
    uploadAndPublish(update: false)
  }
  
  func sharingTagsViewControllerDidCancel(_ viewController: SharingTagsViewController) {
    navigationController?.popViewController(animated: true)
  }
}

extension BookmarksSharingViewController: SharingPropertiesViewControllerDelegate {
  func sharingPropertiesViewController(_ viewController: SharingPropertiesViewController,
                                       didSelect userStatus: BookmarkGroupAuthorType) {
    sharingUserStatus = userStatus
    showSelectTags()
  }
}

extension BookmarksSharingViewController: EditOnWebViewControllerDelegate {
  func editOnWebViewControllerDidFinish(_ viewController: EditOnWebViewController) {
    navigationController?.popViewController(animated: true)
  }
}

extension BookmarksSharingViewController: GuideSharingNameViewControllerDelegate {
  func viewController(_ viewController: GuideSharingNameViewController, didFinishEditing text: String) {
    manager.setCategory(category.categoryId, name: text)
    showEditDescr()
  }
}

extension BookmarksSharingViewController: GuideSharingDescriptionViewControllerDelegate {
  func viewController(_ viewController: GuideSharingDescriptionViewController, didFinishEditing text: String) {
    manager.setCategory(category.categoryId, description: text)
    showSelectProperties()
  }
}
