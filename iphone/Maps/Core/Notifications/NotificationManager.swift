@objc
protocol NotificationManagerDelegate {
  func didOpenNotification(_ notification: Notification)
}

@objc
final class NotificationManager: NSObject {
  @objc var delegate: NotificationManagerDelegate?

  @objc
  func showNotification(_ notification: Notification) {
    let notificationContent = UNMutableNotificationContent()
    notificationContent.title = notification.title
    notificationContent.body = notification.text
    notificationContent.sound = UNNotificationSound.default
    notificationContent.userInfo = notification.userInfo
    let notificationRequest = UNNotificationRequest(identifier: notification.identifier,
                                                    content: notificationContent,
                                                    trigger: UNTimeIntervalNotificationTrigger(timeInterval: 1,
                                                                                               repeats: false))
    UNUserNotificationCenter.current().add(notificationRequest)
  }
}

extension NotificationManager: UNUserNotificationCenterDelegate {
  func userNotificationCenter(_ center: UNUserNotificationCenter,
                              willPresent notification: UNNotification,
                              withCompletionHandler completionHandler: @escaping (UNNotificationPresentationOptions) -> Void) {
    if Notification.fromUserInfo(notification.request.content.userInfo) != nil {
      completionHandler([.sound, .alert])
    } else {
      MWMPushNotifications.userNotificationCenter(center,
                                                  willPresent: notification,
                                                  withCompletionHandler: completionHandler)
    }
  }

  func userNotificationCenter(_ center: UNUserNotificationCenter,
                              didReceive response: UNNotificationResponse,
                              withCompletionHandler completionHandler: @escaping () -> Void) {
    if let n = Notification.fromUserInfo(response.notification.request.content.userInfo) {
      delegate?.didOpenNotification(n)
    } else {
      MWMPushNotifications.userNotificationCenter(center,
                                                  didReceive: response,
                                                  withCompletionHandler: completionHandler)
    }
  }
}
