@objc(MWMUGCTextReviewDelegate)
protocol UGCTextReviewDelegate: NSObjectProtocol {
  func changeReviewText(_ text: String)
}

@objc(MWMUGCTextReviewCell)
final class UGCTextReviewCell: MWMTableViewCell, UITextViewDelegate {
  private enum Consts {
    static let kMaxNumberOfSymbols = 400
  }

  @IBOutlet private weak var textView: MWMTextView!
  @IBOutlet private weak var countLabel: UILabel!
  private weak var delegate: UGCTextReviewDelegate?
  private var indexPath: NSIndexPath = NSIndexPath()

  func configWith(delegate: UGCTextReviewDelegate?) {
    self.delegate = delegate
    setCount(textView.text.characters.count)
  }

  private func setCount(_ count: Int) {
    countLabel.text = "\(count)/\(Consts.kMaxNumberOfSymbols)"
  }

  // MARK: UITextViewDelegate
  func textView(_ textView: UITextView, shouldChangeTextIn _: NSRange, replacementText _: String) -> Bool {
    return textView.text.characters.count <= Consts.kMaxNumberOfSymbols
  }

  func textViewDidChange(_ textView: UITextView) {
    setCount(textView.text.characters.count)
  }

  func textViewDidEndEditing(_ textView: UITextView) {
    delegate?.changeReviewText(textView.text)
  }
}
