@objcMembers
final class MailComposer: NSObject {
  private static let mailComposer = MailComposer()
  private static var topViewController: UIViewController { .topViewController() }

  override private init() {}

  /// Composes an email with the provided subject, body and attachment file for the given recipients.
  static func sendEmail(subject: String? = nil, body: String? = nil, toRecipients recipients: [String], attachmentFileURL: URL? = nil) {
    sendEmailWith(subject: subject ?? "",
                  body: body ?? "",
                  toRecipients: recipients,
                  attachment: attachmentFileURL.flatMap(Attachment.init(fileURL:)))
  }

  /// Composes an email with the additional app information and the log file attachment for the developers.
  static func sendBugReportWith(title: String) {
    func subject() -> String {
      let appInfo = AppInfo.shared()
      return String(format: "[%@-%@ iOS] %@", appInfo.bundleVersion, appInfo.buildNumber, title)
    }

    func body() -> String {
      let appInfo = AppInfo.shared()
      return String(format: "\n\n\n\n- %@ (%@)\n- Organic Maps %@-%@\n- %@-%@\n- %@\n",
                    appInfo.deviceModel, UIDevice.current.systemVersion,
                    appInfo.bundleVersion, appInfo.buildNumber,
                    Locale.current.languageCode ?? "",
                    Locale.current.regionCode ?? "",
                    Locale.preferredLanguages.joined(separator: ", "))
    }
    UIApplication.shared.showLoadingOverlay {
      // The completion runs on a background queue: read the archive and drop it there, and go back
      // to the main queue only for the UI.
      Logger.getLogFileURL { logFileURL in
        let attachment = logFileURL.flatMap(Attachment.init(fileURL:))
        if let logFileURL {
          // The logger creates the archive in a temporary directory of its own.
          try? FileManager.default.removeItem(at: logFileURL.deletingLastPathComponent())
        }
        DispatchQueue.main.async {
          UIApplication.shared.hideLoadingOverlay {
            sendEmailWith(subject: subject(),
                          body: body(),
                          toRecipients: [SocialMedia.organicMapsEmail.link],
                          attachment: attachment)
          }
        }
      }
    }
  }

  private struct Attachment {
    let data: Data
    let fileName: String
    let mimeType = "application/zip"

    init?(fileURL: URL) {
      guard let data = try? Data(contentsOf: fileURL) else {
        LOG(.error, "Failed to read the attachment at \(fileURL)")
        return nil
      }
      self.data = data
      fileName = fileURL.lastPathComponent
    }
  }

  private static func sendEmailWith(subject: String, body: String, toRecipients recipients: [String], attachment: Attachment? = nil) {
    // If the attachment is provided, the default mail composer should be used.
    if let attachment {
      if MWMMailViewController.canSendMail() {
        let mailViewController = MWMMailViewController()
        mailViewController.mailComposeDelegate = mailComposer
        mailViewController.setSubject(subject)
        mailViewController.setMessageBody(body, isHTML: false)
        mailViewController.setToRecipients(recipients)
        mailViewController.addAttachmentData(attachment.data, mimeType: attachment.mimeType, fileName: attachment.fileName)
        topViewController.present(mailViewController, animated: true, completion: nil)
      } else {
        showMailComposingAlert(recipients: recipients)
      }
      return
    }

    // From iOS 14, it is possible to change the default mail app, and mailto should open a default mail app.
    if !openDefaultMailApp(subject: subject, body: body, recipients: recipients) {
      showMailComposingAlert(recipients: recipients)
    }
  }

  private static func openOutlook(subject: String, body: String, recipients: [String]) -> Bool {
    var components = URLComponents(string: "ms-outlook://compose")!
    components.queryItems = [
      URLQueryItem(name: "to", value: recipients.joined(separator: ";")),
      URLQueryItem(name: "subject", value: subject),
      URLQueryItem(name: "body", value: body),
    ]

    if let url = components.url, UIApplication.shared.canOpenURL(url) {
      UIApplication.shared.open(url)
      return true
    }
    return false
  }

  private static func openGmail(subject: String, body: String, recipients: [String]) -> Bool {
    var components = URLComponents(string: "googlegmail://co")!
    components.queryItems = [
      URLQueryItem(name: "to", value: recipients.joined(separator: ";")),
      URLQueryItem(name: "subject", value: subject),
      URLQueryItem(name: "body", value: body),
    ]

    if let url = components.url, UIApplication.shared.canOpenURL(url) {
      UIApplication.shared.open(url)
      return true
    }
    return false
  }

  private static func openDefaultMailApp(subject: String, body: String, recipients: [String]) -> Bool {
    var components = URLComponents(string: "mailto:\(recipients.joined(separator: ";"))")
    components?.queryItems = [
      URLQueryItem(name: "subject", value: subject),
      URLQueryItem(name: "body", value: body.replacingOccurrences(of: "\n", with: "\r\n")),
    ]

    if let url = components?.url, UIApplication.shared.canOpenURL(url) {
      UIApplication.shared.open(url)
      return true
    }
    return false
  }

  private static func showMailComposingAlert(recipients: [String]) {
    let text = String(format: L("email_error_body"), recipients.joined(separator: ";"))
    let alert = UIAlertController(title: L("email_error_title"), message: text, preferredStyle: .alert)
    let action = UIAlertAction(title: L("ok"), style: .default, handler: nil)
    alert.addAction(action)
    topViewController.present(alert, animated: true, completion: nil)
  }
}

// MARK: - MFMailComposeViewControllerDelegate

extension MailComposer: MFMailComposeViewControllerDelegate {
  func mailComposeController(_ controller: MFMailComposeViewController, didFinishWith _: MFMailComposeResult, error _: Error?) {
    controller.dismiss(animated: true, completion: nil)
  }
}
