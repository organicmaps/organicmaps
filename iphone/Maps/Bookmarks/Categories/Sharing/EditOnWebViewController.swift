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
    
    Statistics.logEvent(kStatEditOnWebClick, withParameters: [kStatItem : kStatCopyLink])
    let message = String(coreFormat: L("share_bookmarks_email_body_link"), arguments: [guide.absoluteString])
    let shareController = MWMActivityViewController.share(for: nil, message: message) {
      [weak self] _, success, _, _ in
      if success {
        Statistics.logEvent(kStatSharingLinkSuccess, withParameters: [kStatFrom : kStatEditOnWeb])
      }
      if let self = self {
        self.delegate?.editOnWebViewControllerDidFinish(self)
      }
    }
    shareController?.present(inParentViewController: self, anchorView: sendMeLinkButton)
  }
  
  @IBAction func cancelButtonPressed(_ sender: Any) {
    delegate?.editOnWebViewControllerDidFinish(self)
  }
}
