typealias OpenInApplicationCompletionHandler = (OpenInApplication) -> Void

extension UIAlertController {
  static func presentInAppActionSheet(from sourceView: UIView,
                                      apps: [OpenInApplication] = OpenInApplication.availableApps,
                                      didSelectApp: @escaping OpenInApplicationCompletionHandler) -> UIAlertController {
    let alertController = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)

    apps.forEach { app in
      let action = UIAlertAction(title: app.name, style: .default) { _ in
        didSelectApp(app)
      }
      alertController.addAction(action)
    }

    let cancelAction = UIAlertAction(title: L("cancel"), style: .cancel)
    alertController.addAction(cancelAction)

    iPadSpecific {
      alertController.popoverPresentationController?.sourceView = sourceView
      alertController.popoverPresentationController?.sourceRect = sourceView.bounds
    }
    return alertController
  }
}
