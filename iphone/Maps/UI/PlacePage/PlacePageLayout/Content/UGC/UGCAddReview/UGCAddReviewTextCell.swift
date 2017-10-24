final class UGCAddReviewTextCell: MWMTableViewCell {
  private enum Const {
    static let maxCharactersCount = 600
  }

  @IBOutlet private weak var textView: MWMTextView! {
    didSet {
      textView.placeholder = L("placepage_reviews_hint")
      textView.font = UIFont.regular16()
      textView.textColor = UIColor.blackPrimaryText()
      textView.placeholderView.textColor = UIColor.blackSecondaryText()
    }
  }

  var reviewText: String! {
    get { return textView.text }
    set { textView.text = newValue }
  }
}

extension UGCAddReviewTextCell: UITextViewDelegate {
  func textView(_: UITextView, shouldChangeTextIn range: NSRange, replacementText text: String) -> Bool {
    let isNewLengthValid = reviewText.count + text.count - range.length <= Const.maxCharactersCount
    return isNewLengthValid
  }
}
