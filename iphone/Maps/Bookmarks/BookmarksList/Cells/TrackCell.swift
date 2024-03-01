final class TrackCell: UITableViewCell {
  @IBOutlet private var trackImageView: UIImageView!
  @IBOutlet private var trackTitleLabel: UILabel!
  @IBOutlet private var trackSubtitleLabel: UILabel!

  private var trackColorDidTapAction: (() -> Void)?

  override func awakeFromNib() {
    super.awakeFromNib()
    let tapGesture = UITapGestureRecognizer(target: self, action: #selector(colorDidTapAction(_:)))
    trackImageView.addGestureRecognizer(tapGesture)
  }

  func config(_ track: ITrackViewModel) {
    trackImageView.image = track.image
    trackTitleLabel.text = track.trackName
    trackSubtitleLabel.text = track.subtitle
    trackColorDidTapAction = track.colorDidTapAction
  }

  @objc private func colorDidTapAction(_ sender: UITapGestureRecognizer) {
    trackColorDidTapAction?()
  }
}
