import SafariServices

protocol BookmarksSharingViewControllerDelegate: AnyObject {
  func didShareCategory()
}

final class BookmarksSharingViewController: MWMTableViewController {
  typealias ViewModel = MWMAuthorizationViewModel
  
  var categoryId: MWMMarkGroupID?
  var categoryUrl: URL?
  weak var delegate: BookmarksSharingViewControllerDelegate?
  
  @IBOutlet weak var uploadAndPublishCell: UploadActionCell!
  @IBOutlet weak var getDirectLinkCell: UploadActionCell!
  
  let kPropertiesSegueIdentifier = "chooseProperties"
  let kTagsControllerIdentifier = "tags"
  
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
    
    title = L("sharing_options") //"Sharing options"
    configureActionCells()
    
    assert(categoryId != nil, "We can't share nothing")
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
                             image: UIImage(named: "ic24PxLink"), delegate: self)
  }
  
  override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    return UITableViewAutomaticDimension
  }
  
  override func tableView(_ tableView: UITableView,
                 titleForHeaderInSection section: Int) -> String? {
    return section == 0 ? L("public_access") : L("limited_access")
  }
  
  override func tableView(_ tableView: UITableView,
                 willSelectRowAt indexPath: IndexPath) -> IndexPath? {
    let cell = tableView.cellForRow(at: indexPath)
    if cell == getDirectLinkCell {
      if getDirectLinkCell.cellState != .normal {
        return nil
      }
    }
    
    return indexPath
  }
  
  override func tableView(_ tableView: UITableView,
                          didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    
    let cell = tableView.cellForRow(at: indexPath)
    if cell == uploadAndPublishCell {
      uploadAndPublish()
    } else if cell == getDirectLinkCell {
      uploadAndGetDirectLink()
    }
  }
  
  func uploadAndPublish() {
    performAfterValidation { [weak self] in
      self?.performSegue(withIdentifier: "chooseProperties", sender: self)
    }
  }
  
  override func prepare(for segue: UIStoryboardSegue, sender: Any?)  {
    if segue.identifier == kPropertiesSegueIdentifier {
      if let vc = segue.destination as? SharingPropertiesViewController {
        vc.delegate = self
      }
    }
  }
  
  func uploadAndGetDirectLink() {
    performAfterValidation { [weak self] in
      guard let categoryId = self?.categoryId else {
        assert(false, "categoryId must not be nil")
        return
      }
      
      MWMBookmarksManager.shared().uploadAndGetDirectLinkCategory(withId: categoryId, progress: { (progress) in
        if progress == .uploadStarted {
          self?.getDirectLinkCell.cellState = .inProgress
        }
      }, completion: { (url, error) in
        if error != nil {
          self?.getDirectLinkCell.cellState = .normal
          //TODO: handle errors
        } else {
          self?.getDirectLinkCell.cellState = .completed
          self?.categoryUrl = url
          self?.delegate?.didShareCategory()
        }
      })
    }
  }
  
  func performAfterValidation(action: @escaping MWMVoidBlock) {
    MWMFrameworkHelper.checkConnectionAndPerformAction { [weak self] in
      if let self = self, let view = self.view {
        self.signup(anchor: view, onComplete: { success in
          if success {
            action()
          }
        })
      }
    }
  }
}

extension BookmarksSharingViewController: UITextViewDelegate {
  func textView(_ textView: UITextView, shouldInteractWith URL: URL, in characterRange: NSRange) -> Bool {
    let safari = SFSafariViewController(url: URL)
    present(safari, animated: true, completion: nil)
    return false;
  }
}

extension BookmarksSharingViewController: UploadActionCellDelegate {
  func cellDidPressShareButton(_ cell: UploadActionCell) {
    let message = L("share_bookmarks_email_body")
    let shareController = MWMActivityViewController.share(for: categoryUrl, message: message)
    shareController?.present(inParentViewController: self, anchorView: nil)
  }
}

extension BookmarksSharingViewController: SharingTagsViewControllerDelegate {
  func sharingTagsViewController(_ viewController: SharingTagsViewController, didSelect tags: [MWMTag]) {
    navigationController?.popViewController(animated: true)
    // TODO: proceed with selected tags
  }
  
  func sharingTagsViewControllerDidCancel(_ viewController: SharingTagsViewController) {
    navigationController?.popViewController(animated: true)
  }
}

extension BookmarksSharingViewController: SharingPropertiesViewControllerDelegate {
  func sharingPropertiesViewController(_ viewController: SharingPropertiesViewController,
                                       didSelect userStatus: SharingPropertyUserStatus) {
    // TODO: proceed with chosen user status
    
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
