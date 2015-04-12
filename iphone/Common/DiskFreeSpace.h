#pragma once

uint64_t FreeDiskSpaceInBytes()
{
  NSError * error = nil;
  NSArray * paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSDictionary * dictionary = [[NSFileManager defaultManager] attributesOfFileSystemForPath:[paths lastObject] error: &error];
  
  if (dictionary)
  {
    NSNumber * freeFileSystemSizeInBytes = [dictionary objectForKey: NSFileSystemFreeSize];
    return [freeFileSystemSizeInBytes longLongValue];
  }
  else
  {
    NSLog(@"Error Obtaining Free File System Info: Domain = %@, Code = %ld", [error domain], (long)[error code]);
    return 0;
  }
}
