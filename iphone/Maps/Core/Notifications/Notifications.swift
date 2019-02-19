@objc
class Notification: NSObject {
  fileprivate static let kLocalNotificationName = "LocalNotificationName"
  fileprivate static let kLocalNotificationTitle = "LocalNotificationTitle"
  fileprivate static let kLocalNotificationText = "LocalNotificationText"
  let title: String
  let text: String
  var userInfo: [AnyHashable : Any] {
    return [Notification.kLocalNotificationName : identifier,
            Notification.kLocalNotificationTitle : title,
            Notification.kLocalNotificationText : text]
  }

  var identifier: String {
    assert(false, "Override in subclass")
    return ""
  }

  @objc
  init(title: String, text: String) {
    self.title = title
    self.text = text
  }

  convenience init?(userInfo: [AnyHashable : Any]) {
    if let title = userInfo[Notification.kLocalNotificationTitle] as? String,
      let text = userInfo[Notification.kLocalNotificationText] as? String {
      self.init(title: title, text: text)
    } else {
      return nil
    }
  }

  class func fromUserInfo(_ userInfo: [AnyHashable : Any]) -> Notification? {
    if let notificationName = userInfo[Notification.kLocalNotificationName] as? String {
      switch notificationName {
      case ReviewNotification.identifier:
        return ReviewNotification(userInfo: userInfo)
      case AuthNotification.identifier:
        return AuthNotification(userInfo: userInfo)
      default:
        return nil
      }
    }
    return nil
  }
}

@objc
class AuthNotification: Notification {
  static let identifier = "UGC"
  override var identifier: String { return AuthNotification.identifier }
}

@objc
class ReviewNotification: Notification {
  private static let kNotificationObject = "NotificationObject"
  static let identifier = "ReviewNotification"
  @objc let notificationWrapper: CoreNotificationWrapper
  override var identifier: String { return ReviewNotification.identifier }
  override var userInfo: [AnyHashable : Any] {
    var result = super.userInfo
    result[ReviewNotification.kNotificationObject] = notificationWrapper.notificationDictionary()
    return result
  }

  @objc
  init(title: String, text: String, notificationWrapper: CoreNotificationWrapper) {
    self.notificationWrapper = notificationWrapper
    super.init(title: title, text: text)
  }

  convenience init?(userInfo: [AnyHashable : Any]) {
    if let title = userInfo[Notification.kLocalNotificationTitle] as? String,
      let text = userInfo[Notification.kLocalNotificationText] as? String,
      let notificationDictionary = userInfo[ReviewNotification.kNotificationObject] as? [AnyHashable : Any],
      let notificationWrapper = CoreNotificationWrapper(notificationDictionary: notificationDictionary) {
      self.init(title: title, text: text, notificationWrapper: notificationWrapper)
    } else {
      return nil
    }
  }
}
