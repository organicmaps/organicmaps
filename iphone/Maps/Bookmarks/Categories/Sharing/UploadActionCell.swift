protocol UploadActionCellDelegate: AnyObject {
  func cellDidPressShareButton(_ cell: UploadActionCell, senderView: UIView)
}

enum UploadActionCellState: String {
  case normal
  case inProgress
  case updating
  case completed
  case disabled
}

final class UploadActionCell: MWMTableViewCell {
  @IBOutlet private weak var actionImage: UIImageView!
  @IBOutlet private weak var actionTitle: UILabel!
  @IBOutlet private weak var shareButton: UIButton!
  @IBOutlet private weak var progressView: UIView!

  weak var delegate: UploadActionCellDelegate?
  private var titles: [UploadActionCellState : String]?

  func config(titles: [UploadActionCellState : String], image: UIImage?, delegate: UploadActionCellDelegate?) {
    actionImage.image = image
    self.titles = titles
    self.delegate = delegate
    self.cellState = .normal
  }
  
  var cellState: UploadActionCellState = .normal {
    didSet {
      switch cellState {
      case .normal:
        progressView.isHidden = true
        actionImage.setStyleAndApply("MWMBlue")
        actionTitle.setStyleAndApply("regular16:linkBlueText")
        actionTitle.text = titles?[.normal]
        shareButton.isHidden = true
        selectionStyle = .default
      case .inProgress:
        progressView.isHidden = false
        actionImage.setStyleAndApply("MWMBlack")
        actionTitle.setStyleAndApply("italic16:blackSecondaryText")
        actionTitle.text = titles?[.inProgress]
        shareButton.isHidden = true
        selectionStyle = .none
      case .updating:
        progressView.isHidden = false
        actionImage.setStyleAndApply("MWMBlack")
        actionTitle.setStyleAndApply("italic16:blackSecondaryText")
        actionTitle.text = titles?[.updating]
        shareButton.isHidden = true
        selectionStyle = .none
      case .completed:
        progressView.isHidden = true
        actionImage.setStyleAndApply("MWMBlack")
        actionTitle.setStyleAndApply("regular16:blackSecondaryText")
        actionTitle.text = titles?[.completed]
        shareButton.isHidden = false
        selectionStyle = .none
      case .disabled:
        progressView.isHidden = true
        actionImage.setStyleAndApply("MWMBlack")
        actionTitle.setStyleAndApply("regular16:blackSecondaryText")
        actionTitle.text = titles?[.disabled]
        shareButton.isHidden = true
        selectionStyle = .none
      }
    }
  }
  
  @IBAction func shareButtonPressed(_ sender: Any) {
    assert(cellState == .completed, "Share button can only be pressed while the cell is in .completed state")
    delegate?.cellDidPressShareButton(self, senderView: shareButton)
  }
}
