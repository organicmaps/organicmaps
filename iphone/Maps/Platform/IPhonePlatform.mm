#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/NSBundle.h>
#import <Foundation/NSPathUtilities.h>
#import <Foundation/NSProcessInfo.h>
#import "IPhonePlatform.hpp"
#import <UIKit/UIDevice.h>
#import <UIKit/UIScreen.h>
#import <UIKit/UIScreenMode.h>

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

/// @return path to data stored inside application
char const * ResourcesPath()
{
  NSBundle * bundle = [NSBundle mainBundle];
  NSString * path = [bundle resourcePath];
  return [path UTF8String];
}

IPhonePlatform::IPhonePlatform()
{
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];    

  m_StartDate = [[NSDate alloc] init];
  
  // create symlinks for resource files, like skins, classificator etc.
	char const * files[] = {"symbols_24.png",		
													"symbols_48.png",
													"dejavusans_8.png", 
													"dejavusans_10.png",
													"dejavusans_12.png",
													"dejavusans_14.png",
													"dejavusans_16.png",		
													"dejavusans_20.png",		
													"dejavusans_24.png",		
													"basic.skn",
													"basic_highres.skn",
													"drawing_rules.bin", 
													"classificator.txt",
													"minsk-pass.dat", 
													"minsk-pass.dat.idx",
													"visibility.txt"};
      // TODO: change liechtenstein to world
      
  std::string resPath(ResourcesPath());
  resPath += '/';
  std::string workPath(WorkingDir());
  workPath += '/';
  for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); ++i)
  {
  	unlink((workPath + files[i]).c_str());
    int res = symlink((resPath + files[i]).c_str(), (workPath + files[i]).c_str());
    res += 2;
  }

	/// Hardcoding screen resolution depending on the device we are running.
 	m_visualScale = 1.0;
	m_skinName = "basic.skn";
	m_isMultiSampled = true;

	/// Calculating resolution
	UIDevice * device = [UIDevice currentDevice];
	
	NSRange range = [device.name rangeOfString:@"iPad"];
	if (range.location != NSNotFound)
		m_visualScale = 1.3;
	else {
		range = [device.name rangeOfString:@"iPod"];
		float ver = [device.systemVersion floatValue];
		if (ver >= 3.1999)
		{
			UIScreen * mainScr = [UIScreen mainScreen];
			if (mainScr.currentMode.size.width == 640)
			{
				m_visualScale = 2.0;
				m_isMultiSampled = false;
				m_skinName = "basic_highres.skn";
			}
		}
	}
	
	NSLog(@"Device Name : %@, SystemName : %@, SystemVersion : %@", device.name, device.systemName, device.systemVersion);


  [pool release];
}

double IPhonePlatform::TimeInSec()
{
  NSDate * now = [[NSDate alloc] init];
  double interval = [now timeIntervalSinceDate:m_StartDate];
  [now release];
  return interval;
}

string IPhonePlatform::ResourcesDir()
{
	return WorkingDir();
}

string IPhonePlatform::WorkingDir()
{
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
  NSArray * dirPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
  NSString * docsDir = [dirPaths objectAtIndex:0];
  std::string wdPath([docsDir UTF8String]);
  [pool release];
  return wdPath + "/";
}

int IPhonePlatform::GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles)
{
  DIR * dir;
  struct dirent * entry;

  if ((dir = opendir(directory.c_str())) == NULL)
    return 0;

	// TODO: take wildcards into account...
	string mask_fixed = mask;
  if (mask_fixed.size() && mask_fixed[0] == '*')
  	mask_fixed.erase(0, 1);
  
  do
  {
    if ((entry = readdir(dir)) != NULL)
    {
      string fname(entry->d_name);
      size_t index = fname.rfind(mask_fixed);
      if (index != string::npos && index == fname.size() - mask_fixed.size())
      {
      	// TODO: By some strange reason under simulator stat returns -1, may be because of symbolic links?..
//        struct stat fileStatus;
//        if (stat(string(directory + fname).c_str(), &fileStatus) == 0
//          && !(fileStatus.st_mode & S_IFDIR))
//        {
          outFiles.push_back(fname);
//        }
      }
    }
  } while (entry != NULL);

  closedir(dir);
  
  return outFiles.size();
}

bool IPhonePlatform::GetFileSize(string const & file, uint64_t & size)
{
	struct stat fileStatus;
  if (stat(file.c_str(), &fileStatus) == 0)
  {
		size = fileStatus.st_size;
    return true;
  }
  return false;
}

bool IPhonePlatform::RenameFileX(string const & original, string const & newName)
{
  return rename(original.c_str(), newName.c_str());
}

int IPhonePlatform::CpuCores()
{
	NSInteger numCPU = [[NSProcessInfo processInfo] activeProcessorCount];
  if (numCPU >= 1)
  	return numCPU;
	return 1;
}

string const IPhonePlatform::SkinName() const
{
	return m_skinName;
}

double IPhonePlatform::VisualScale() const
{
  return m_visualScale;
}

bool IPhonePlatform::IsMultiSampled() const
{
	return m_isMultiSampled;
}

Platform & GetPlatform()
{
  static IPhonePlatform platform;
  return platform;
}
