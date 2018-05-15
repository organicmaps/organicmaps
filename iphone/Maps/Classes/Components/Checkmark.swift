//
//  Checkmark.swift
//  MAPS.ME
//
//  Created by Aleksey Belousov on 15/05/2018.
//  Copyright © 2018 MapsWithMe. All rights reserved.
//

import UIKit

@IBDesignable
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
    imageView.center = CGPoint(x: bounds.width / 2, y: bounds.height / 2)
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
