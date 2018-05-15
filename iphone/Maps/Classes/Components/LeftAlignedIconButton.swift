//
//  LeftAlignedIconButton.swift
//  MAPS.ME
//
//  Created by Aleksey Belousov on 15/05/2018.
//  Copyright © 2018 MapsWithMe. All rights reserved.
//

import UIKit

@IBDesignable
class LeftAlignedIconButton: UIButton {
  override func layoutSubviews() {
    super.layoutSubviews()
    contentHorizontalAlignment = .left
    let availableSpace = UIEdgeInsetsInsetRect(bounds, contentEdgeInsets)
    let imageWidth = imageView?.frame.width ?? 0
    let titleWidth = titleLabel?.frame.width ?? 0
    let availableWidth = availableSpace.width - imageEdgeInsets.right - imageWidth - titleWidth
    titleEdgeInsets = UIEdgeInsets(top: 0, left: availableWidth / 2, bottom: 0, right: 0)
  }
}

