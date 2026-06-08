enum ColorPickerType {
  case defaultColorPicker(UIColor?)
}

final class ColorPicker: NSObject {
  static let shared = ColorPicker()

  private var onUpdateColorHandler: ((UIColor) -> Void)?

  // MARK: - Public

  /// Presents the native arbitrary-color picker modally from a specified root view controller.
  /// Used for both bookmarks and tracks (and on iOS-app-on-Mac).
  func present(from rootViewController: UIViewController, pickerType: ColorPickerType, completionHandler: ((UIColor) -> Void)?) {
    onUpdateColorHandler = completionHandler
    switch pickerType {
    case .defaultColorPicker(let color):
      rootViewController.present(defaultColorPickerViewController(with: color), animated: true)
    }
  }

  // MARK: - Private

  private func defaultColorPickerViewController(with selectedColor: UIColor?) -> UIViewController {
    let colorPickerController = UIColorPickerViewController()
    colorPickerController.supportsAlpha = false
    if let selectedColor {
      colorPickerController.selectedColor = selectedColor
    }
    colorPickerController.delegate = self
    colorPickerController.presentationController?.delegate = self
    return colorPickerController
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
