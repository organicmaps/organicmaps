#ifndef FileManagerBridge_h
#define FileManagerBridge_h

#ifdef __cplusplus
extern "C" {
#endif

void copyFileToDocuments(const char* fileName, const char* fileExtension, const char* subdirectory);

#ifdef __cplusplus
}
#endif

#endif /* FileManagerBridge_h */
