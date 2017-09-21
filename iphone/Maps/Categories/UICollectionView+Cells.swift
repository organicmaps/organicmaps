extension UICollectionView {
  @objc func register(cellClass: AnyClass) {
    register(UINib(cellClass), forCellWithReuseIdentifier: toString(cellClass))
  }

  @objc func dequeueReusableCell(withCellClass cellClass: AnyClass, indexPath: IndexPath) -> UICollectionViewCell {
    return dequeueReusableCell(withReuseIdentifier: toString(cellClass), for: indexPath)
  }
}
