protocol UploadActionCellDelegate: AnyObject {
  func cellDidPressShareButton(_ cell: UploadActionCell, senderView: UIView)
}

enum UploadActionCellState: String {
  case normal
  case inProgress
  case completed
}

final class UploadActionCell: MWMTableViewCell {
  @IBOutlet private weak var actionImage: UIImageView!
  @IBOutlet private weak var actionTitle: UILabel!
  @IBOutlet private weak var progressView: UIView!
  @IBOutlet private weak var shareButton: UIButton!
  
  weak var delegate: UploadActionCellDelegate?
  private var titles: [UploadActionCellState : String]?
  private lazy var progress: MWMCircularProgress = {
    return MWMCircularProgress(parentView: self.progressView)
  }()
  
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
        progress.state = .normal
        actionImage.tintColor = .linkBlue()
        actionTitle.textColor = .linkBlue()
        actionTitle.font = .regular16()
        actionTitle.text = titles?[.normal]
        shareButton.isHidden = true
        break
      case .inProgress:
        progress.state = .spinner
        actionImage.tintColor = .blackSecondaryText()
        actionTitle.textColor = .blackSecondaryText()
        actionTitle.font = .italic16()
        actionTitle.text = titles?[.inProgress]
        shareButton.isHidden = true
        break
      case .completed:
        progress.state = .completed
        actionImage.tintColor = .blackSecondaryText()
        actionTitle.textColor = .blackSecondaryText()
        actionTitle.font = .regular16()
        actionTitle.text = titles?[.completed]
        shareButton.isHidden = false
        break
      }
    }
  }
  
  @IBAction func shareButtonPressed(_ sender: Any) {
    assert(cellState == .completed, "Share button can only be pressed while the cell is in .completed state")
    delegate?.cellDidPressShareButton(self, senderView: shareButton)
  }
}
