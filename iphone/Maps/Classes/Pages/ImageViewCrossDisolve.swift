
class ImageViewCrossDisolve: UIView {
  private var imageViews: [UIImageView] = []
  var images: [UIImage?] = [] {
    didSet{
      imageViews.forEach { (imageView) in
        imageView.removeFromSuperview();
      }
      for image in images{
        let imageView = UIImageView(image: image);
        self.addSubview(imageView)
        imageView.alpha = 0
        imageView.contentMode = .scaleAspectFill
        imageView.translatesAutoresizingMaskIntoConstraints = false;
        imageView.leadingAnchor.constraint(equalTo: self.leadingAnchor).isActive = true
        imageView.trailingAnchor.constraint(equalTo: self.trailingAnchor).isActive = true
        imageView.topAnchor.constraint(equalTo: self.topAnchor).isActive = true
        imageView.trailingAnchor.constraint(equalTo: self.trailingAnchor).isActive = true
        self.imageViews.append(imageView)
      }
      updateLayout()
    }
  }
  var pageCount: Int {
    return images.count
  }
  var currentPage: CGFloat = 0.0 {
    didSet {
      updateLayout()
    }
  }

  private func updateLayout() {
    for i in 0..<imageViews.count {
      let imageView = imageViews[i]
      let progress:CGFloat = currentPage - CGFloat(i)
      let alpha = max(CGFloat(0.0), min(CGFloat(1.0), progress+1))
      imageView.alpha = alpha
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    updateLayout()
  }
}
