final class TrackCell: UITableViewCell {
  @IBOutlet var trackImageView: UIImageView!
  @IBOutlet var trackTitleLabel: UILabel!
  @IBOutlet var trackSubtitleLabel: UILabel!

  func config(_ track: ITrackViewModel) {
    trackImageView.image = track.image
    trackTitleLabel.text = track.trackName
    trackSubtitleLabel.text = track.subtitle
  }
}
