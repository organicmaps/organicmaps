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
    
    if MWMMailViewController.canSendMail() {
      let mailController = MWMMailViewController()
      mailController.mailComposeDelegate = self
      mailController.setSubject(L("edit_guide_title"))
      
      let text = String(format: "%@\n\n%@", L("edit_your_guide_email_body"), guide.absoluteString)
      mailController.setMessageBody(text, isHTML: false)
      mailController.navigationBar.titleTextAttributes = [
        NSAttributedStringKey.foregroundColor : UIColor.white
      ]
      self.present(mailController, animated: true, completion: nil)
    } else {
      MWMAlertViewController.activeAlert().presentInfoAlert(L("email_error_title"),
                                                            text: L("email_error_body"))
    }
  }
  
  @IBAction func cancelButtonPressed(_ sender: Any) {
    delegate?.editOnWebViewControllerDidFinish(self)
  }
}

extension EditOnWebViewController: MFMailComposeViewControllerDelegate {
  func mailComposeController(_ controller: MFMailComposeViewController,
                             didFinishWith result: MFMailComposeResult, error: Error?) {
    delegate?.editOnWebViewControllerDidFinish(self)
  }
}
