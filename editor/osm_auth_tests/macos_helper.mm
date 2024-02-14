#include <QCoreApplication>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

static QCoreApplication *app;

// This function creates main queue and runs func() in a separate thread
void runMainQueue(void func()) {
  dispatch_queue_t customQueue = dispatch_queue_create("custom.queue", DISPATCH_QUEUE_CONCURRENT);

  dispatch_async(customQueue, ^{
    func();
    // Stop main queue
    app->exit();
  });

  int argc = 0;
  char **argv = nullptr;

  // Start main queue
  app = new QCoreApplication(argc, argv);
  app->exec();
}
