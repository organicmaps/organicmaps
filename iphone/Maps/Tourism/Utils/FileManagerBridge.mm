#import "FileManagerBridge.h"
#import "SwiftBridge.h"

void copyFileToDocuments(const char* fileName, const char* fileExtension, const char* subdirectory) {
    NSString *nsFileName = [NSString stringWithUTF8String:fileName];
    NSString *nsFileExtension = [NSString stringWithUTF8String:fileExtension];
    NSString *nsSubdirectory = [NSString stringWithUTF8String:subdirectory];

    [FileManagerHelper copyProjectFileToDocumentsWithFileName:nsFileName
                                                fileExtension:nsFileExtension
                                                toSubdirectory:nsSubdirectory];
}

