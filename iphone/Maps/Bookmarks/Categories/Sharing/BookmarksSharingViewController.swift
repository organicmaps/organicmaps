import SafariServices

final class BookmarksSharingViewController: MWMTableViewController {
  typealias ViewModel = MWMAuthorizationViewModel
  
  @IBOutlet weak var uploadAndPublishCell: UploadActionCell!
  @IBOutlet weak var getDirectLinkCell: UploadActionCell!
  
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
    self.configureActionCells()
  }
  
  func configureActionCells() {
    uploadAndPublishCell.config(title: L("upload_and_publish"), image: UIImage(named: "ic24PxGlobe")!)
    getDirectLinkCell.config(title: L("upload_and_get_direct_link"), image: UIImage(named: "ic24PxLink")!)
  }
  
  override func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
    return UITableViewAutomaticDimension
  }
  
  override func tableView(_ tableView: UITableView,
                 titleForHeaderInSection section: Int) -> String? {
    return section == 0 ? L("public_access") : L("limited_access")
  }
  
  override func tableView(_ tableView: UITableView,
                          didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    
    let cell = tableView.cellForRow(at: indexPath)
    if (cell == self.uploadAndPublishCell) {
      self.uploadAndPublish()
    } else if (cell == self.getDirectLinkCell) {
      self.getDirectLink()
    }
  }
  
  func uploadAndPublish() {
    self.performAfterValidation {
      //implementation
    }
  }
  
  func getDirectLink() {
    self.performAfterValidation {
      //implementation
    }
  }
  
  func performAfterValidation(action: @escaping MWMVoidBlock) {
    MWMFrameworkHelper.checkConnectionAndPerformAction { [unowned self] in
      self.signup(anchor: self.view, onComplete: { success in
        if (success) {
          action()
        }
      })
    }
  }
}

extension BookmarksSharingViewController: UITextViewDelegate {
  func textView(_ textView: UITextView, shouldInteractWith URL: URL, in characterRange: NSRange) -> Bool {
    let safari = SFSafariViewController(url: URL)
    self.present(safari, animated: true, completion: nil)
    return false;
  }
}
