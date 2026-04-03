typealias URLsCompletionHandler = ([URL]) -> Void

final class DocumentPicker: NSObject {
  static let shared = DocumentPicker()
  private var completionHandler: URLsCompletionHandler?

  func present(from rootViewController: UIViewController,
               fileTypes: [FileType] = [.kml, .kmz, .gpx, .geoJson, .json],
               completionHandler: @escaping URLsCompletionHandler) {
    self.completionHandler = completionHandler
    let documentPickerViewController = UIDocumentPickerViewController(forOpeningContentTypes: fileTypes.map(\.utType), asCopy: true)
    documentPickerViewController.delegate = self
    // TODO: Enable multiple selection when the multiple files parsing support will be added to the bookmark_manager.
    documentPickerViewController.allowsMultipleSelection = false
    rootViewController.present(documentPickerViewController, animated: true)
  }
}

// MARK: - UIDocumentPickerDelegate

extension DocumentPicker: UIDocumentPickerDelegate {
  func documentPicker(_: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
    completionHandler?(urls)
  }
}
