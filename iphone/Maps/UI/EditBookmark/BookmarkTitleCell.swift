import UIKit

protocol BookmarkTitleCellDelegate: AnyObject {
  func didFinishEditingTitle(_ title: String)
}

final class BookmarkTitleCell: MWMTableViewCell {
  @IBOutlet var textField: UITextField!
  weak var delegate: BookmarkTitleCellDelegate?

  func configure(name: String, delegate: BookmarkTitleCellDelegate) {
    textField.text = name
    self.delegate = delegate
  }
}

extension BookmarkTitleCell: UITextFieldDelegate {
  func textFieldDidEndEditing(_ textField: UITextField) {
    delegate?.didFinishEditingTitle(textField.text ?? "")
  }
}
