@objc (MWMSpinnerAlert)
final class SpinnerAlert: MWMAlert {
  @IBOutlet private weak var progressView: UIView!
  @IBOutlet private weak var title: UILabel!
  @IBOutlet private weak var cancelHeight: NSLayoutConstraint!
  @IBOutlet private weak var cancelButton: UIButton!
  @IBOutlet private weak var divider: UIView!

  private var cancel: MWMVoidBlock?

  private lazy var progress: MWMCircularProgress = {
    var p = MWMCircularProgress.downloaderProgress(forParentView: progressView)
    return p
  }()


  @objc static func alert(title: String, cancel: MWMVoidBlock?) -> SpinnerAlert? {
    guard let alert = Bundle.main.loadNibNamed(className(), owner: nil, options: nil)?.first
      as? SpinnerAlert else {
      assertionFailure()
      return nil
    }

    alert.title.text = title
    alert.progress.state = .spinner
    alert.progress.setCancelButtonHidden()

    if let cancel = cancel {
      alert.cancel = cancel
    } else {
      alert.cancelHeight.constant = 0
      alert.cancelButton.isHidden = true
      alert.divider.isHidden = true
      alert.setNeedsLayout()
    }

    return alert
  }

  @IBAction private func tap() {
    close(cancel)
  }
}
