protocol EditOnWebViewControllerDelegate: AnyObject {
  func editOnWebViewControllerDidFinish(_ viewController: EditOnWebViewController)
}

final class EditOnWebViewController: MWMViewController {
  weak var delegate: EditOnWebViewControllerDelegate?
  var guideUrl: URL?
  
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
  
  @IBAction func sendMeLinkButtonPressed(_ sender: Any) {
    guard let guide = guideUrl else {
      assert(false, "must provide guide url")
      return
    }
    
    let message = L("share_bookmarks_email_body")
    let shareController = MWMActivityViewController.share(for: guide, message: message) {
      [weak self] _, _, _, _ in
      if let self = self {
        self.delegate?.editOnWebViewControllerDidFinish(self)
      }
    }
    shareController?.present(inParentViewController: self, anchorView: nil)
  }
  
  @IBAction func cancelButtonPressed(_ sender: Any) {
    delegate?.editOnWebViewControllerDidFinish(self)
  }
}
