
class ImageViewCrossDisolve: UIView {
  private var imageViews: [UIImageView] = []
  var images: [UIImage?] = [] {
    didSet {
      for imageView in imageViews {
        imageView.removeFromSuperview()
      }
      for image in images {
        let imageView = UIImageView(image: image)
        addSubview(imageView)
        imageView.alpha = 0
        imageView.contentMode = .scaleAspectFill
        imageView.translatesAutoresizingMaskIntoConstraints = false
        imageView.leadingAnchor.constraint(equalTo: leadingAnchor).isActive = true
        imageView.trailingAnchor.constraint(equalTo: trailingAnchor).isActive = true
        imageView.topAnchor.constraint(equalTo: topAnchor).isActive = true
        imageView.trailingAnchor.constraint(equalTo: trailingAnchor).isActive = true
        imageViews.append(imageView)
      }
      updateLayout()
    }
  }

  var pageCount: Int {
    images.count
  }

  var currentPage: CGFloat = 0.0 {
    didSet {
      updateLayout()
    }
  }

  private func updateLayout() {
    for i in 0 ..< imageViews.count {
      let imageView = imageViews[i]
      let progress: CGFloat = currentPage - CGFloat(i)
      let alpha = max(CGFloat(0.0), min(CGFloat(1.0), progress + 1))
      imageView.alpha = alpha
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    updateLayout()
  }
}
