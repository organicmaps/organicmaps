extension UICollectionView {
  @objc func register(cellClass: AnyClass) {
    register(UINib(cellClass), forCellWithReuseIdentifier: toString(cellClass))
  }

  @objc func dequeueReusableCell(withCellClass cellClass: AnyClass, indexPath: IndexPath) -> UICollectionViewCell {
    dequeueReusableCell(withReuseIdentifier: toString(cellClass), for: indexPath)
  }

  func register<Cell: UICollectionViewCell>(cell: Cell.Type) {
    register(cell, forCellWithReuseIdentifier: toString(cell))
  }

  func dequeueReusableCell<Cell: UICollectionViewCell>(cell: Cell.Type, indexPath: IndexPath) -> Cell {
    dequeueReusableCell(withReuseIdentifier: toString(cell), for: indexPath) as! Cell
  }
}
