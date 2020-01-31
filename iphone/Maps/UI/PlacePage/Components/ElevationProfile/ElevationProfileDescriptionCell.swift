class ElevationProfileDescriptionCell: UICollectionViewCell {
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var valueLabel: UILabel!
  @IBOutlet var imageView: UIImageView!
  
  func configure(title: String, value: String, imageName: String) {
    titleLabel.text = title
    valueLabel.text = value
    imageView.image = UIImage(named: imageName)
  }
  
  override func prepareForReuse() {
    super.prepareForReuse()
    titleLabel.text = ""
    valueLabel.text = ""
    imageView.image = nil
  }
}
