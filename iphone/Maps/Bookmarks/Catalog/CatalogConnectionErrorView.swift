class CatalogConnectionErrorView: UIView {
  
  @IBOutlet var imageView: UIImageView!
  @IBOutlet var titleLabel: UILabel!
  @IBOutlet var actionButton: UIButton!
  var actionCallback: (() -> Void)?
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    xibSetup()
  }
  
  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    xibSetup()
  }
  
  convenience init(frame: CGRect, actionCallback callback: (() -> Void)?) {
    self.init(frame: frame)
    actionCallback = callback
  }
  
  @IBAction func actionTapHandler() {
    actionCallback?()
  }

}

extension CatalogConnectionErrorView {
  func xibSetup() {
    let nib = UINib(nibName: "CatalogConnectionErrorView", bundle: nil)
    let view = nib.instantiate(withOwner: self, options: nil).first as! UIView
    view.translatesAutoresizingMaskIntoConstraints = false
    addSubview(view)
    addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "H:|[childView]|",
                                                  options: [],
                                                  metrics: nil,
                                                  views: ["childView": view]))
    addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "V:|[childView]|",
                                                  options: [],
                                                  metrics: nil,
                                                  views: ["childView": view]))
  }
}
