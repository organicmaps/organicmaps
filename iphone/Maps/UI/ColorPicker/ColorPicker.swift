enum ColorPickerType {
  case defaultColorPicker(UIColor?)
  case bookmarkColorPicker(BookmarkColor?)
}

final class ColorPicker: NSObject {
  static let shared = ColorPicker()

  private var onUpdateColorHandler: ((UIColor) -> Void)?

  // MARK: - Public

  /// Presents a color picker view controller modally from a specified root view controller.
  ///
  /// - Uses native color picker on the iOS 14.0+ for the `defaultColorPicker` type on iPhone and iPad.
  /// - For the rest of the iOS versions, `bookmarkColorPicker` type and iPad designed for Mac uses a custom color picker.
  ///
  func present(from rootViewController: UIViewController, pickerType: ColorPickerType, completionHandler: ((UIColor) -> Void)?) {
    onUpdateColorHandler = completionHandler
    let colorPickerViewController: UIViewController

    switch pickerType {
    case .defaultColorPicker(let color):
      if !ProcessInfo.processInfo.isiOSAppOnMac {
        colorPickerViewController = defaultColorPickerViewController(with: color)
      } else {
        var selectedColor: BookmarkColor?
        if let color {
          selectedColor = BookmarkColor.bookmarkColor(from: color)
        }
        colorPickerViewController = bookmarksColorPickerViewController(with: selectedColor)
      }
    case .bookmarkColorPicker(let bookmarkColor):
      colorPickerViewController = bookmarksColorPickerViewController(with: bookmarkColor)
    }
    rootViewController.present(colorPickerViewController, animated: true)
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

  private func bookmarksColorPickerViewController(with selectedColor: BookmarkColor?) -> UIViewController {
    let bookmarksColorViewController = BookmarkColorViewController(bookmarkColor: selectedColor)
    bookmarksColorViewController.delegate = self
    // The navigation controller is used for getting the navigation item with the title and the close button.
    return UINavigationController(rootViewController: bookmarksColorViewController)
  }

  private func commitSelection(_ color: UIColor) {
    guard let onUpdateColorHandler else { return }
    self.onUpdateColorHandler = nil
    onUpdateColorHandler(color)
  }
}

// MARK: - BookmarkColorViewControllerDelegate

extension ColorPicker: BookmarkColorViewControllerDelegate {
  func bookmarkColorViewController(_ viewController: BookmarkColorViewController, didSelect bookmarkColor: BookmarkColor) {
    commitSelection(bookmarkColor.color)
    viewController.dismiss(animated: true)
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
