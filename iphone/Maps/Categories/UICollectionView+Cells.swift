extension UICollectionView {
  @objc func register(cellClass: AnyClass) {
    register(UINib(cellClass), forCellWithReuseIdentifier: toString(cellClass))
  }

  @objc func dequeueReusableCell(withCellClass cellClass: AnyClass, indexPath: IndexPath) -> UICollectionViewCell {
    return dequeueReusableCell(withReuseIdentifier: toString(cellClass), for: indexPath)
  }

  func register<Cell>(cell: Cell.Type) where Cell: UICollectionViewCell {
    register(cell, forCellWithReuseIdentifier: toString(cell))
  }

  func dequeueReusableCell<Cell>(cell: Cell.Type, indexPath: IndexPath) -> Cell where Cell: UICollectionViewCell {
    return dequeueReusableCell(withReuseIdentifier: toString(cell), for: indexPath) as! Cell
  }
}
