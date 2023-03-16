final class TrackCell: UITableViewCell {
  @IBOutlet private var trackImageView: UIImageView!
  @IBOutlet private var trackTitleLabel: UILabel!
  @IBOutlet private var trackSubtitleLabel: UILabel!

  func config(_ track: ITrackViewModel) {
    trackImageView.image = track.image
    trackTitleLabel.text = track.trackName
    trackSubtitleLabel.text = track.subtitle
  }
}
