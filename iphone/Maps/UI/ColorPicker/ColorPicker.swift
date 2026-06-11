final class ColorPicker: NSObject {
  static let shared = ColorPicker()

  private var onUpdateColorHandler: ((UIColor) -> Void)?

  /// Presents the native arbitrary-color picker modally from a specified root view controller.
  /// Used for both bookmarks and tracks (and on iOS-app-on-Mac).
  /// `currentColor` preselects the color in the palette; pass nil when there is no current
  /// color (e.g. when selecting a color for the whole group).
  func present(from rootViewController: UIViewController, currentColor: UIColor?, completionHandler: ((UIColor) -> Void)?) {
    let colorPickerController = UIColorPickerViewController()
    colorPickerController.supportsAlpha = false
    if let currentColor {
      colorPickerController.selectedColor = currentColor
    }
    colorPickerController.delegate = self
    colorPickerController.presentationController?.delegate = self
    onUpdateColorHandler = completionHandler
    rootViewController.present(colorPickerController, animated: true)
  }

  private func commitSelection(_ color: UIColor) {
    guard let onUpdateColorHandler else { return }
    self.onUpdateColorHandler = nil
    onUpdateColorHandler(color)
  }
}

// MARK: - UIColorPickerViewControllerDelegate

extension ColorPicker: UIColorPickerViewControllerDelegate {
  func colorPickerViewControllerDidFinish(_ viewController: UIColorPickerViewController) {
    commitSelection(viewController.selectedColor.sRGBColor)
  }
}

// MARK: - UIAdaptivePresentationControllerDelegate

extension ColorPicker: UIAdaptivePresentationControllerDelegate {
  func presentationControllerDidDismiss(_ presentationController: UIPresentationController) {
    guard let colorPickerViewController = presentationController.presentedViewController as? UIColorPickerViewController else {
      return
    }
    commitSelection(colorPickerViewController.selectedColor.sRGBColor)
  }
}
