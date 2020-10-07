class Checkmark: UIControl {
  
  private let imageView = UIImageView(frame: .zero)
  
  @IBInspectable
  var offImage: UIImage? {
    didSet {
      updateImage(animated: false)
    }
  }
  
  @IBInspectable
  var onImage: UIImage? {
    didSet {
      updateImage(animated: false)
    }
  }
  
  @IBInspectable
  var offTintColor: UIColor? {
    didSet {
      updateTintColor()
    }
  }
  
  @IBInspectable
  var onTintColor: UIColor? {
    didSet {
      updateTintColor()
    }
  }
  
  @IBInspectable
  var isChecked: Bool = false {
    didSet {
      updateImage(animated: true)
      updateTintColor()
    }
  }
  
  override var isHighlighted: Bool {
    didSet {
      imageView.tintColor = isHighlighted ? tintColor.blending(with: UIColor(white: 0, alpha: 0.5)) : nil
    }
  }
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    initViews()
  }
  
  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    initViews()
  }
  
  private func initViews() {
    addSubview(imageView)
    addTarget(self, action: #selector(onTouch), for: .touchUpInside)
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    
    imageView.sizeToFit()
    
    var left: CGFloat = 0;
    var top: CGFloat = 0;
    var width: CGFloat = imageView.width;
    var height: CGFloat = imageView.height;
    
    switch contentHorizontalAlignment {
    case .right: fallthrough
    case .trailing:
      left = floor(bounds.width - imageView.width)
    case .center:
      left = floor((bounds.width - width) / 2)
    case .fill:
      width = bounds.width
    default:
      left = 0
    }
    
    switch contentVerticalAlignment {
    case .top:
      top = 0
    case .bottom:
      top = floor(bounds.height - height)
    case .center:
      top = floor((bounds.height - height) / 2)
    case .fill:
      height = bounds.height
    }
    
    imageView.frame = CGRect(x: left, y: top, width: width, height: height)
  }
  
  @objc func onTouch() {
    isChecked = !isChecked
    sendActions(for: .valueChanged)
  }
  
  private func updateImage(animated: Bool) {
    self.imageView.image = self.isChecked ? self.onImage : self.offImage
  }
  
  private func updateTintColor() {
    tintColor = isChecked ? onTintColor : offTintColor
  }
}
