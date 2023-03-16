#import "MWMStorage+UI.h"
#import "MWMAlertViewController.h"

@implementation MWMStorage (UI)

- (void)handleError:(NSError *)error {
  if (error.code == kStorageNotEnoughSpace) {
    [[MWMAlertViewController activeAlertController] presentNotEnoughSpaceAlert];
  } else if (error.code == kStorageNoConnection) {
    [[MWMAlertViewController activeAlertController] presentNoConnectionAlert];
  } else if (error.code == kStorageRoutingActive) {
    [[MWMAlertViewController activeAlertController] presentDeleteMapProhibitedAlert];
  } else {
    NSAssert(NO, @"Unknown error code");
  }
}

- (void)downloadNode:(NSString *)countryId {
  [self downloadNode:countryId onSuccess:nil];
}

- (void)downloadNode:(NSString *)countryId onSuccess:(MWMVoidBlock)success {
  NSError *error;
  [self downloadNode:countryId error:&error];
  if (error) {
    if (error.code == kStorageCellularForbidden) {
      __weak __typeof(self) ws = self;
      [[MWMAlertViewController activeAlertController] presentNoWiFiAlertWithOkBlock:^{
        [self enableCellularDownload:YES];
        [ws downloadNode:countryId];
      } andCancelBlock:nil];
    } else {
      [self handleError:error];
    }
    return;
  }
  if (success) {
    success();
  }
}

- (void)updateNode:(NSString *)countryId {
  [self updateNode:countryId onCancel:nil];
}

- (void)updateNode:(NSString *)countryId onCancel:(MWMVoidBlock)cancel {
  NSError *error;
  [self updateNode:countryId error:&error];
  if (error) {
    if (error.code == kStorageCellularForbidden) {
      __weak __typeof(self) ws = self;
      [[MWMAlertViewController activeAlertController] presentNoWiFiAlertWithOkBlock:^{
        [self enableCellularDownload:YES];
        [ws updateNode:countryId onCancel:cancel];
      } andCancelBlock:cancel];
    } else {
      [self handleError:error];
      if (cancel) {
        cancel();
      }
    }
  }
}

- (void)deleteNode:(NSString *)countryId {
  [self deleteNode:countryId ignoreUnsavedEdits:NO];
}

- (void)deleteNode:(NSString *)countryId ignoreUnsavedEdits:(BOOL)force {
  NSError *error;
  [self deleteNode:countryId ignoreUnsavedEdits:force error:&error];
  if (error) {
    __weak __typeof(self) ws = self;
    if (error.code == kStorageCellularForbidden) {
      [[MWMAlertViewController activeAlertController] presentNoWiFiAlertWithOkBlock:^{
        [self enableCellularDownload:YES];
        [ws deleteNode:countryId];
      } andCancelBlock:nil];
    } else if (error.code == kStorageHaveUnsavedEdits) {
      [[MWMAlertViewController activeAlertController] presentUnsavedEditsAlertWithOkBlock:^ {
        [ws deleteNode:countryId ignoreUnsavedEdits:YES];
      }];
    } else {
      [self handleError:error];
    }
  }
}

- (void)downloadNodes:(NSArray<NSString *> *)countryIds onSuccess:(nullable MWMVoidBlock)success {
  NSError *error;
  [self downloadNodes:countryIds error:&error];
  if (error) {
    if (error.code == kStorageCellularForbidden) {
      __weak __typeof(self) ws = self;
      [[MWMAlertViewController activeAlertController] presentNoWiFiAlertWithOkBlock:^{
        [self enableCellularDownload:YES];
        [ws downloadNodes:countryIds onSuccess:success];
      } andCancelBlock:nil];
    } else {
      [self handleError:error];
    }
    return;
  }
  if (success) {
    success();
  }
}

@end
