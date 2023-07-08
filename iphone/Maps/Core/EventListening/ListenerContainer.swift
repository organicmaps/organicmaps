import Foundation

// TODO: define directly T as AnyObject after Swift version update
final class ListenerContainer<T> {
  // MARK: - WeakWrapper for listeners
  private class WeakWrapper<T> {
    private weak var weakValue: AnyObject?
    
    init(value: T) {
      self.weakValue = value as AnyObject?
    }
    
    var value: T? {
      return weakValue as? T
    }
  }
  
  // MARK: - Properties
  private var listeners = [WeakWrapper<T>]()
  
  // MARK: - Public methods
  func addListener(_ listener: T) {
    guard isUnique(listener) else {
      return
    }
    listeners.append(WeakWrapper(value: listener))
  }
  
  func removeListener(_ listener: T) {
    listeners = listeners.filter({ weakRef in
      guard let object = weakRef.value else {
        return false
      }
      return !identical(object, listener)
    })
  }
  
  func forEach(_ block: @escaping (T) -> Void) {
    fetchListeners().forEach(block)
  }
  
  // MARK: - Private methods
  private func isUnique(_ listener: T) -> Bool {
    return !fetchListeners().contains(where: { identical($0, listener) })
  }
  
  private func fetchListeners() -> [T] {
    removeNilReference()
    return listeners.compactMap({ $0.value })
  }
  
  private func removeNilReference() {
    listeners = listeners.filter({ $0.value != nil })
  }
}

private func identical(_ lhs: Any, _ rhs: Any) -> Bool {
  return (lhs as AnyObject?) === (rhs as AnyObject?)
}

