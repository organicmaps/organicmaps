//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_WIN32_SYNC_PRIMITIVES_HPP
#define BOOST_INTERPROCESS_WIN32_SYNC_PRIMITIVES_HPP

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <cstddef>
#include <cstring>
#include <string>
#include <memory>

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#  pragma comment( lib, "advapi32.lib" )
#endif

#if (defined BOOST_INTERPROCESS_WINDOWS)
#  include <cstdarg>
#  include <boost/detail/interlocked.hpp>
#else
# error "This file can only be included in Windows OS"
#endif

//The structures used in Interprocess with the
//same binary interface as windows ones
namespace boost {
namespace interprocess {
namespace winapi {

//Some used constants
static const unsigned long infinite_time        = 0xFFFFFFFF;
static const unsigned long error_already_exists = 183L;
static const unsigned long error_sharing_violation = 32L;
static const unsigned long error_file_not_found = 2u;
static const unsigned long error_no_more_files  = 18u;
//Retries in CreateFile, see http://support.microsoft.com/kb/316609
static const unsigned int  error_sharing_violation_tries = 3u;
static const unsigned int  error_sharing_violation_sleep_ms = 250u;

static const unsigned long semaphore_all_access = (0x000F0000L)|(0x00100000L)|0x3;
static const unsigned long mutex_all_access     = (0x000F0000L)|(0x00100000L)|0x0001;

static const unsigned long page_readonly        = 0x02;
static const unsigned long page_readwrite       = 0x04;
static const unsigned long page_writecopy       = 0x08;

static const unsigned long standard_rights_required   = 0x000F0000L;
static const unsigned long section_query              = 0x0001;
static const unsigned long section_map_write          = 0x0002;
static const unsigned long section_map_read           = 0x0004;
static const unsigned long section_map_execute        = 0x0008;
static const unsigned long section_extend_size        = 0x0010;
static const unsigned long section_all_access         = standard_rights_required |
                                                        section_query            |
                                                        section_map_write        |
                                                        section_map_read         |
                                                        section_map_execute      |
                                                        section_extend_size;

static const unsigned long file_map_copy        = section_query;
static const unsigned long file_map_write       = section_map_write;
static const unsigned long file_map_read        = section_map_read;
static const unsigned long file_map_all_access  = section_all_access;
static const unsigned long delete_access = 0x00010000L;
static const unsigned long file_flag_backup_semantics = 0x02000000;
static const long file_flag_delete_on_close = 0x04000000;

//Native API constants
static const unsigned long file_open_for_backup_intent = 0x00004000;
static const int file_share_valid_flags = 0x00000007;
static const long file_delete_on_close = 0x00001000L;
static const long obj_case_insensitive = 0x00000040L;

static const unsigned long movefile_copy_allowed            = 0x02;
static const unsigned long movefile_delay_until_reboot      = 0x04;
static const unsigned long movefile_replace_existing        = 0x01;
static const unsigned long movefile_write_through           = 0x08;
static const unsigned long movefile_create_hardlink         = 0x10;
static const unsigned long movefile_fail_if_not_trackable   = 0x20;

static const unsigned long file_share_read      = 0x00000001;
static const unsigned long file_share_write     = 0x00000002;
static const unsigned long file_share_delete    = 0x00000004;

static const unsigned long file_attribute_readonly    = 0x00000001;
static const unsigned long file_attribute_hidden      = 0x00000002;
static const unsigned long file_attribute_system      = 0x00000004;
static const unsigned long file_attribute_directory   = 0x00000010;
static const unsigned long file_attribute_archive     = 0x00000020;
static const unsigned long file_attribute_device      = 0x00000040;
static const unsigned long file_attribute_normal      = 0x00000080;
static const unsigned long file_attribute_temporary   = 0x00000100;

static const unsigned long generic_read         = 0x80000000L;
static const unsigned long generic_write        = 0x40000000L;

static const unsigned long wait_object_0        = 0;
static const unsigned long wait_abandoned       = 0x00000080L;
static const unsigned long wait_timeout         = 258L;
static const unsigned long wait_failed          = (unsigned long)0xFFFFFFFF;

static const unsigned long duplicate_close_source  = (unsigned long)0x00000001;
static const unsigned long duplicate_same_access   = (unsigned long)0x00000002;

static const unsigned long format_message_allocate_buffer
   = (unsigned long)0x00000100;
static const unsigned long format_message_ignore_inserts
   = (unsigned long)0x00000200;
static const unsigned long format_message_from_string
   = (unsigned long)0x00000400;
static const unsigned long format_message_from_hmodule
   = (unsigned long)0x00000800;
static const unsigned long format_message_from_system
   = (unsigned long)0x00001000;
static const unsigned long format_message_argument_array
   = (unsigned long)0x00002000;
static const unsigned long format_message_max_width_mask
   = (unsigned long)0x000000FF;
static const unsigned long lang_neutral         = (unsigned long)0x00;
static const unsigned long sublang_default      = (unsigned long)0x01;
static const unsigned long invalid_file_size    = (unsigned long)0xFFFFFFFF;
static       void * const  invalid_handle_value = (void*)(long)(-1);
static const unsigned long create_new        = 1;
static const unsigned long create_always     = 2;
static const unsigned long open_existing     = 3;
static const unsigned long open_always       = 4;
static const unsigned long truncate_existing = 5;

static const unsigned long file_begin     = 0;
static const unsigned long file_current   = 1;
static const unsigned long file_end       = 2;

static const unsigned long lockfile_fail_immediately  = 1;
static const unsigned long lockfile_exclusive_lock    = 2;
static const unsigned long error_lock_violation       = 33;
static const unsigned long security_descriptor_revision = 1;

//Own defines
static const long SystemTimeOfDayInfoLength  = 48;
static const long BootAndSystemstampLength   = 16;
static const long BootstampLength            = 8;
static const unsigned long MaxPath           = 260;

//Keys
static void * const  hkey_local_machine = (void*)(unsigned long*)(long)(0x80000002);
static unsigned long key_query_value    = 0x0001;

}  //namespace winapi {
}  //namespace interprocess  {
}  //namespace boost  {

#if !defined( BOOST_USE_WINDOWS_H )

namespace boost  {
namespace interprocess  {
namespace winapi {

struct interprocess_overlapped 
{
   unsigned long *internal;
   unsigned long *internal_high;
   union {
      struct {
         unsigned long offset;
         unsigned long offset_high;
      }dummy;
      void *pointer;
   };

   void *h_event;
};

struct interprocess_filetime
{  
   unsigned long  dwLowDateTime;  
   unsigned long  dwHighDateTime;
};

struct win32_find_data_t
{
   unsigned long dwFileAttributes;
   interprocess_filetime ftCreationTime;
   interprocess_filetime ftLastAccessTime;
   interprocess_filetime ftLastWriteTime;
   unsigned long nFileSizeHigh;
   unsigned long nFileSizeLow;
   unsigned long dwReserved0;
   unsigned long dwReserved1;
   char cFileName[MaxPath];
   char cAlternateFileName[14];
};

struct interprocess_security_attributes
{
   unsigned long nLength;
   void *lpSecurityDescriptor;
   int bInheritHandle;
};

struct system_info {
    union {
        unsigned long dwOemId;          // Obsolete field...do not use
        struct {
            unsigned short wProcessorArchitecture;
            unsigned short wReserved;
        } dummy;
    };
    unsigned long dwPageSize;
    void * lpMinimumApplicationAddress;
    void * lpMaximumApplicationAddress;
    unsigned long * dwActiveProcessorMask;
    unsigned long dwNumberOfProcessors;
    unsigned long dwProcessorType;
    unsigned long dwAllocationGranularity;
    unsigned short wProcessorLevel;
    unsigned short wProcessorRevision;
};

struct interprocess_memory_basic_information
{
   void *         BaseAddress;  
   void *         AllocationBase;
   unsigned long  AllocationProtect;
   unsigned long  RegionSize;
   unsigned long  State;
   unsigned long  Protect;
   unsigned long  Type;
};

typedef struct _interprocess_acl
{
   unsigned char  AclRevision;
   unsigned char  Sbz1;
   unsigned short AclSize;
   unsigned short AceCount;
   unsigned short Sbz2;
} interprocess_acl;

typedef struct _interprocess_security_descriptor
{
   unsigned char Revision;
   unsigned char Sbz1;
   unsigned short Control;
   void *Owner;
   void *Group;
   interprocess_acl *Sacl;
   interprocess_acl *Dacl;
} interprocess_security_descriptor;

enum file_information_class_t {
   file_directory_information = 1,
   file_full_directory_information,
   file_both_directory_information,
   file_basic_information,
   file_standard_information,
   file_internal_information,
   file_ea_information,
   file_access_information,
   file_name_information,
   file_rename_information,
   file_link_information,
   file_names_information,
   file_disposition_information,
   file_position_information,
   file_full_ea_information,
   file_mode_information,
   file_alignment_information,
   file_all_information,
   file_allocation_information,
   file_end_of_file_information,
   file_alternate_name_information,
   file_stream_information,
   file_pipe_information,
   file_pipe_local_information,
   file_pipe_remote_information,
   file_mailslot_query_information,
   file_mailslot_set_information,
   file_compression_information,
   file_copy_on_write_information,
   file_completion_information,
   file_move_cluster_information,
   file_quota_information,
   file_reparse_point_information,
   file_network_open_information,
   file_object_id_information,
   file_tracking_information,
   file_ole_directory_information,
   file_content_index_information,
   file_inherit_content_index_information,
   file_ole_information,
   file_maximum_information
};

struct file_name_information_t {
   unsigned long FileNameLength;
   wchar_t FileName[1];
};

struct file_rename_information_t {
   int Replace;
   void *RootDir;
   unsigned long FileNameLength;
   wchar_t FileName[1];
};

struct unicode_string_t {
   unsigned short Length;
   unsigned short MaximumLength;
   wchar_t *Buffer;
};

struct object_attributes_t {
   unsigned long Length;
   void * RootDirectory;
   unicode_string_t *ObjectName;
   unsigned long Attributes;
   void *SecurityDescriptor;
   void *SecurityQualityOfService;
};

struct io_status_block_t {
   union {
      long Status;
      void *Pointer;
   };

   unsigned long *Information;
};

union system_timeofday_information
{
   struct data_t
   {
      __int64 liKeBootTime;
      __int64 liKeSystemTime;
      __int64 liExpTimeZoneBias;
      unsigned long uCurrentTimeZoneId;
      unsigned long dwReserved;
   } data;
   unsigned char Reserved1[SystemTimeOfDayInfoLength];
};

enum system_information_class {
   system_basic_information = 0,
   system_performance_information = 2,
   system_time_of_day_information = 3,
   system_process_information = 5,
   system_processor_performance_information = 8,
   system_interrupt_information = 23,
   system_exception_information = 33,
   system_registry_quota_information = 37,
   system_lookaside_information = 45
};

enum object_information_class
{
   object_basic_information,
   object_name_information,
   object_type_information,
   object_all_information,
   object_data_information
};

struct object_name_information_t
{
   unicode_string_t Name;
   wchar_t NameBuffer[1];
};

//Some windows API declarations
extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentProcessId();
extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId();
extern "C" __declspec(dllimport) void __stdcall Sleep(unsigned long);
extern "C" __declspec(dllimport) unsigned long __stdcall GetLastError();
extern "C" __declspec(dllimport) void * __stdcall GetCurrentProcess();
extern "C" __declspec(dllimport) int __stdcall CloseHandle(void*);
extern "C" __declspec(dllimport) int __stdcall DuplicateHandle
   ( void *hSourceProcessHandle,    void *hSourceHandle
   , void *hTargetProcessHandle,    void **lpTargetHandle
   , unsigned long dwDesiredAccess, int bInheritHandle
   , unsigned long dwOptions);
extern "C" __declspec(dllimport) void *__stdcall FindFirstFileA(const char *lpFileName, win32_find_data_t *lpFindFileData);
extern "C" __declspec(dllimport) int   __stdcall FindNextFileA(void *hFindFile, win32_find_data_t *lpFindFileData);
extern "C" __declspec(dllimport) int   __stdcall FindClose(void *hFindFile);
extern "C" __declspec(dllimport) void __stdcall GetSystemTimeAsFileTime(interprocess_filetime*);
extern "C" __declspec(dllimport) int  __stdcall FileTimeToLocalFileTime(const interprocess_filetime *in, const interprocess_filetime *out);
extern "C" __declspec(dllimport) void * __stdcall CreateMutexA(interprocess_security_attributes*, int, const char *);
extern "C" __declspec(dllimport) void * __stdcall OpenMutexA(unsigned long, int, const char *);
extern "C" __declspec(dllimport) unsigned long __stdcall WaitForSingleObject(void *, unsigned long);
extern "C" __declspec(dllimport) int __stdcall ReleaseMutex(void *);
extern "C" __declspec(dllimport) int __stdcall UnmapViewOfFile(void *);
extern "C" __declspec(dllimport) void * __stdcall CreateSemaphoreA(interprocess_security_attributes*, long, long, const char *);
extern "C" __declspec(dllimport) int __stdcall ReleaseSemaphore(void *, long, long *);
extern "C" __declspec(dllimport) void * __stdcall OpenSemaphoreA(unsigned long, int, const char *);
extern "C" __declspec(dllimport) void * __stdcall CreateFileMappingA (void *, interprocess_security_attributes*, unsigned long, unsigned long, unsigned long, const char *);
extern "C" __declspec(dllimport) void * __stdcall MapViewOfFileEx (void *, unsigned long, unsigned long, unsigned long, std::size_t, void*);
extern "C" __declspec(dllimport) void * __stdcall OpenFileMappingA (unsigned long, int, const char *);
extern "C" __declspec(dllimport) void * __stdcall CreateFileA (const char *, unsigned long, unsigned long, struct interprocess_security_attributes*, unsigned long, unsigned long, void *);
extern "C" __declspec(dllimport) int __stdcall    DeleteFileA (const char *);
extern "C" __declspec(dllimport) int __stdcall    MoveFileExA (const char *, const char *, unsigned long);
extern "C" __declspec(dllimport) void __stdcall GetSystemInfo (struct system_info *);
extern "C" __declspec(dllimport) int __stdcall FlushViewOfFile (void *, std::size_t);
extern "C" __declspec(dllimport) int __stdcall GetFileSizeEx (void *, __int64 *size);
extern "C" __declspec(dllimport) unsigned long __stdcall FormatMessageA
   (unsigned long dwFlags,       const void *lpSource,   unsigned long dwMessageId, 
   unsigned long dwLanguageId,   char *lpBuffer,         unsigned long nSize, 
   std::va_list *Arguments);
extern "C" __declspec(dllimport) void *__stdcall LocalFree (void *);
extern "C" __declspec(dllimport) int __stdcall CreateDirectoryA(const char *, interprocess_security_attributes*);
extern "C" __declspec(dllimport) int __stdcall RemoveDirectoryA(const char *lpPathName);
extern "C" __declspec(dllimport) int __stdcall GetTempPathA(unsigned long length, char *buffer);
extern "C" __declspec(dllimport) int __stdcall CreateDirectory(const char *, interprocess_security_attributes*);
extern "C" __declspec(dllimport) int __stdcall SetFileValidData(void *, __int64 size);
extern "C" __declspec(dllimport) int __stdcall SetEndOfFile(void *);
extern "C" __declspec(dllimport) int __stdcall SetFilePointerEx(void *, __int64 distance, __int64 *new_file_pointer, unsigned long move_method);
extern "C" __declspec(dllimport) int __stdcall LockFile  (void *hnd, unsigned long offset_low, unsigned long offset_high, unsigned long size_low, unsigned long size_high);
extern "C" __declspec(dllimport) int __stdcall UnlockFile(void *hnd, unsigned long offset_low, unsigned long offset_high, unsigned long size_low, unsigned long size_high);
extern "C" __declspec(dllimport) int __stdcall LockFileEx(void *hnd, unsigned long flags, unsigned long reserved, unsigned long size_low, unsigned long size_high, interprocess_overlapped* overlapped);
extern "C" __declspec(dllimport) int __stdcall UnlockFileEx(void *hnd, unsigned long reserved, unsigned long size_low, unsigned long size_high, interprocess_overlapped* overlapped);
extern "C" __declspec(dllimport) int __stdcall WriteFile(void *hnd, const void *buffer, unsigned long bytes_to_write, unsigned long *bytes_written, interprocess_overlapped* overlapped);
extern "C" __declspec(dllimport) int __stdcall InitializeSecurityDescriptor(interprocess_security_descriptor *pSecurityDescriptor, unsigned long dwRevision);
extern "C" __declspec(dllimport) int __stdcall SetSecurityDescriptorDacl(interprocess_security_descriptor *pSecurityDescriptor, int bDaclPresent, interprocess_acl *pDacl, int bDaclDefaulted);
extern "C" __declspec(dllimport) void *__stdcall LoadLibraryA(const char *);
extern "C" __declspec(dllimport) int   __stdcall FreeLibrary(void *);
extern "C" __declspec(dllimport) void *__stdcall GetProcAddress(void *, const char*);
extern "C" __declspec(dllimport) void *__stdcall GetModuleHandleA(const char*);

//API function typedefs
//Pointer to functions
typedef long (__stdcall *NtDeleteFile_t)(object_attributes_t *ObjectAttributes); 
typedef long (__stdcall *NtSetInformationFile_t)(void *FileHandle, io_status_block_t *IoStatusBlock, void *FileInformation, unsigned long Length, int FileInformationClass ); 
typedef long (__stdcall *NtQueryInformationFile_t)(void *,io_status_block_t *,void *, long, int);
typedef long (__stdcall *NtOpenFile_t)(void*,unsigned long ,object_attributes_t*,io_status_block_t*,unsigned long,unsigned long);
typedef long (__stdcall *NtClose_t) (void*);
typedef long (__stdcall *RtlCreateUnicodeStringFromAsciiz_t)(unicode_string_t *, const char *);
typedef void (__stdcall *RtlFreeUnicodeString_t)(unicode_string_t *);
typedef void (__stdcall *RtlInitUnicodeString_t)( unicode_string_t *, const wchar_t * );
typedef long (__stdcall *RtlAppendUnicodeToString_t)(unicode_string_t *Destination, const wchar_t *Source);
typedef long (__stdcall * NtQuerySystemInformation_t)(int, void*, unsigned long, unsigned long *); 
typedef long (__stdcall * NtQueryObject_t)(void*, object_information_class, void *, unsigned long, unsigned long *); 
typedef unsigned long (__stdcall * GetMappedFileName_t)(void *, void *, wchar_t *, unsigned long);
typedef unsigned long (__stdcall * GetMappedFileName_t)(void *, void *, wchar_t *, unsigned long);
typedef long          (__stdcall * RegOpenKey_t)(void *, const char *, void **);
typedef long          (__stdcall * RegOpenKeyEx_t)(void *, const char *, unsigned long, unsigned long, void **);
typedef long          (__stdcall * RegQueryValue_t)(void *, const char *, char *, long*);
typedef long          (__stdcall * RegQueryValueEx_t)(void *, const char *, unsigned long*, unsigned long*, unsigned char *, unsigned long*);
typedef long          (__stdcall * RegCloseKey_t)(void *);

}  //namespace winapi {
}  //namespace interprocess  {
}  //namespace boost  {

#else
#  include <windows.h>
#endif   //#if !defined( BOOST_USE_WINDOWS_H )

namespace boost {
namespace interprocess {
namespace winapi {

inline unsigned long get_last_error()
{  return GetLastError();  }

inline unsigned long format_message
   (unsigned long dwFlags, const void *lpSource,
    unsigned long dwMessageId, unsigned long dwLanguageId,
    char *lpBuffer, unsigned long nSize, std::va_list *Arguments)
{
   return FormatMessageA
      (dwFlags, lpSource, dwMessageId, dwLanguageId, lpBuffer, nSize, Arguments);
}

//And now, wrapper functions
inline void * local_free(void *hmem)
{  return LocalFree(hmem); }

inline unsigned long make_lang_id(unsigned long p, unsigned long s)
{  return ((((unsigned short)(s)) << 10) | (unsigned short)(p));   }

inline void sched_yield()
{  Sleep(1);   }

inline unsigned long get_current_thread_id()
{  return GetCurrentThreadId();  }

inline unsigned long get_current_process_id()
{  return GetCurrentProcessId();  }

inline unsigned int close_handle(void* handle)
{  return CloseHandle(handle);   }

inline void * find_first_file(const char *lpFileName, win32_find_data_t *lpFindFileData)
{  return FindFirstFileA(lpFileName, lpFindFileData);   }

inline bool find_next_file(void *hFindFile, win32_find_data_t *lpFindFileData)
{  return FindNextFileA(hFindFile, lpFindFileData) != 0;   }

inline bool find_close(void *handle)
{  return FindClose(handle) != 0;   }

inline bool duplicate_current_process_handle
   (void *hSourceHandle, void **lpTargetHandle)
{
   return 0 != DuplicateHandle
      ( GetCurrentProcess(),  hSourceHandle,    GetCurrentProcess()
      , lpTargetHandle,       0,                0
      , duplicate_same_access);
}

inline void get_system_time_as_file_time(interprocess_filetime *filetime)
{  GetSystemTimeAsFileTime(filetime);  }

inline bool file_time_to_local_file_time
   (const interprocess_filetime *in, const interprocess_filetime *out)
{  return 0 != FileTimeToLocalFileTime(in, out);  }

inline void *create_mutex(const char *name)
{  return CreateMutexA(0, 0, name); }

inline void *open_mutex(const char *name)
{  return OpenMutexA(mutex_all_access, 0, name); }

inline unsigned long wait_for_single_object(void *handle, unsigned long time)
{  return WaitForSingleObject(handle, time); }

inline int release_mutex(void *handle)
{  return ReleaseMutex(handle);  }

inline int unmap_view_of_file(void *address)
{  return UnmapViewOfFile(address); }

inline void *create_semaphore(long initialCount, const char *name)
{  return CreateSemaphoreA(0, initialCount, (long)(((unsigned long)(-1))>>1), name);   }

inline int release_semaphore(void *handle, long release_count, long *prev_count)
{  return ReleaseSemaphore(handle, release_count, prev_count); }

inline void *open_semaphore(const char *name)
{  return OpenSemaphoreA(semaphore_all_access, 1, name); }

inline void * create_file_mapping (void * handle, unsigned long access, unsigned long high_size, unsigned long low_size, const char * name)
{
   interprocess_security_attributes sa;
   interprocess_security_descriptor sd; 

   if(!InitializeSecurityDescriptor(&sd, security_descriptor_revision))
      return 0;
   if(!SetSecurityDescriptorDacl(&sd, true, 0, false))
      return 0;
   sa.lpSecurityDescriptor = &sd;
   sa.nLength = sizeof(interprocess_security_attributes);
   sa.bInheritHandle = false;
   return CreateFileMappingA (handle, &sa, access, high_size, low_size, name); 
  //return CreateFileMappingA (handle, 0, access, high_size, low_size, name);  
}

inline void * open_file_mapping (unsigned long access, const char *name)
{  return OpenFileMappingA (access, 0, name);   }

inline void *map_view_of_file_ex(void *handle, unsigned long file_access, unsigned long highoffset, unsigned long lowoffset, std::size_t numbytes, void *base_addr)
{  return MapViewOfFileEx(handle, file_access, highoffset, lowoffset, numbytes, base_addr);  }

inline void *create_file(const char *name, unsigned long access, unsigned long creation_flags, unsigned long attributes = 0)
{
   for (unsigned int attempt(0); attempt < error_sharing_violation_tries; ++attempt){
      void * const handle = CreateFileA(name, access,
                                        file_share_read | file_share_write | file_share_delete,
                                        0, creation_flags, attributes, 0);
      bool const invalid(invalid_handle_value == handle);
      if (!invalid){
         return handle;
      }
      if (error_sharing_violation != get_last_error()){
         return handle;
      }
      Sleep(error_sharing_violation_sleep_ms);
   }
   return invalid_handle_value;
}

inline bool delete_file(const char *name)
{  return 0 != DeleteFileA(name);  }

inline bool move_file_ex(const char *source_filename, const char *destination_filename, unsigned long flags)
{  return 0 != MoveFileExA(source_filename, destination_filename, flags);  }

inline void get_system_info(system_info *info)
{  GetSystemInfo(info); }

inline int flush_view_of_file(void *base_addr, std::size_t numbytes)
{  return FlushViewOfFile(base_addr, numbytes); }

inline bool get_file_size(void *handle, __int64 &size)
{  return 0 != GetFileSizeEx(handle, &size);  }

inline bool create_directory(const char *name, interprocess_security_attributes* security)
{  return 0 != CreateDirectoryA(name, security);   }

inline bool remove_directory(const char *lpPathName)
{  return 0 != RemoveDirectoryA(lpPathName);   }

inline unsigned long get_temp_path(unsigned long length, char *buffer)
{  return GetTempPathA(length, buffer);   }

inline int set_end_of_file(void *handle)
{  return 0 != SetEndOfFile(handle);   }

inline bool set_file_pointer_ex(void *handle, __int64 distance, __int64 *new_file_pointer, unsigned long move_method)
{  return 0 != SetFilePointerEx(handle, distance, new_file_pointer, move_method);   }

inline bool lock_file_ex(void *hnd, unsigned long flags, unsigned long reserved, unsigned long size_low, unsigned long size_high, interprocess_overlapped *overlapped)
{  return 0 != LockFileEx(hnd, flags, reserved, size_low, size_high, overlapped); }

inline bool unlock_file_ex(void *hnd, unsigned long reserved, unsigned long size_low, unsigned long size_high, interprocess_overlapped *overlapped)
{  return 0 != UnlockFileEx(hnd, reserved, size_low, size_high, overlapped);  }

inline bool write_file(void *hnd, const void *buffer, unsigned long bytes_to_write, unsigned long *bytes_written, interprocess_overlapped* overlapped)
{  return 0 != WriteFile(hnd, buffer, bytes_to_write, bytes_written, overlapped);  }

inline long interlocked_increment(long volatile *addr)
{  return BOOST_INTERLOCKED_INCREMENT(addr);  }

inline long interlocked_decrement(long volatile *addr)
{  return BOOST_INTERLOCKED_DECREMENT(addr);  }

inline long interlocked_compare_exchange(long volatile *addr, long val1, long val2)
{  return BOOST_INTERLOCKED_COMPARE_EXCHANGE(addr, val1, val2);  }

inline long interlocked_exchange_add(long volatile* addend, long value)
{  return BOOST_INTERLOCKED_EXCHANGE_ADD(const_cast<long*>(addend), value);  }

inline long interlocked_exchange(long volatile* addend, long value)
{  return BOOST_INTERLOCKED_EXCHANGE(const_cast<long*>(addend), value);  }

//Forward functions
inline void *load_library(const char *name)
{  return LoadLibraryA(name); }

inline bool free_library(void *module)
{  return 0 != FreeLibrary(module); }

inline void *get_proc_address(void *module, const char *name)
{  return GetProcAddress(module, name); }

inline void *get_current_process()
{  return GetCurrentProcess();  }

inline void *get_module_handle(const char *name)
{  return GetModuleHandleA(name); }

inline void initialize_object_attributes
( object_attributes_t *pobject_attr, unicode_string_t *name
 , unsigned long attr, void *rootdir, void *security_descr)

{
   pobject_attr->Length = sizeof(object_attributes_t);
   pobject_attr->RootDirectory = rootdir;
   pobject_attr->Attributes = attr;
   pobject_attr->ObjectName = name;
   pobject_attr->SecurityDescriptor = security_descr;
   pobject_attr->SecurityQualityOfService = 0;
}

inline void rtl_init_empty_unicode_string(unicode_string_t *ucStr, wchar_t *buf, unsigned short bufSize)
{
   ucStr->Buffer = buf;
   ucStr->Length = 0;
   ucStr->MaximumLength = bufSize;
}

//Complex winapi based functions...
struct library_unloader
{
   void *lib_;
   library_unloader(void *module) : lib_(module){}
   ~library_unloader(){ free_library(lib_);  }
};

//pszFilename must have room for at least MaxPath+1 characters
inline bool get_file_name_from_handle_function
   (void * hFile, wchar_t *pszFilename, std::size_t length, std::size_t &out_length) 
{
   if(length <= MaxPath){
      return false;
   }

   void *hiPSAPI = load_library("PSAPI.DLL");
   if (0 == hiPSAPI)
      return 0;

   library_unloader unloader(hiPSAPI);

   //  Pointer to function getMappedFileName() in PSAPI.DLL
   GetMappedFileName_t pfGMFN =
      (GetMappedFileName_t)get_proc_address(hiPSAPI, "GetMappedFileNameW");
   if (! pfGMFN){
      return 0;      //  Failed: unexpected error
   }

   bool bSuccess = false;

   // Create a file mapping object.
   void * hFileMap = create_file_mapping(hFile, page_readonly, 0, 1, 0);
   if(hFileMap)
   {
      // Create a file mapping to get the file name.
      void* pMem = map_view_of_file_ex(hFileMap, file_map_read, 0, 0, 1, 0);

      if (pMem){
         out_length = pfGMFN(get_current_process(), pMem, pszFilename, MaxPath);
         if(out_length){
            bSuccess = true;
         } 
         unmap_view_of_file(pMem);
      }
      close_handle(hFileMap);
   }

   return(bSuccess);
}

inline bool get_system_time_of_day_information(system_timeofday_information &info)
{
   NtQuerySystemInformation_t pNtQuerySystemInformation = (NtQuerySystemInformation_t)
      get_proc_address(get_module_handle("ntdll.dll"), "NtQuerySystemInformation");
   unsigned long res;
   long status = pNtQuerySystemInformation(system_time_of_day_information, &info, sizeof(info), &res);
   if(status){
      return false;
   }
   return true;
}

inline bool get_boot_time(unsigned char (&bootstamp) [BootstampLength])
{
   system_timeofday_information info;
   bool ret = get_system_time_of_day_information(info);
   if(!ret){
      return false;
   }
   std::memcpy(&bootstamp[0], &info.Reserved1, sizeof(bootstamp));
   return true;
}

inline bool get_boot_and_system_time(unsigned char (&bootsystemstamp) [BootAndSystemstampLength])
{
   system_timeofday_information info;
   bool ret = get_system_time_of_day_information(info);
   if(!ret){
      return false;
   }
   std::memcpy(&bootsystemstamp[0], &info.Reserved1, sizeof(bootsystemstamp));
   return true;
}

inline bool get_boot_time_str(char *bootstamp_str, std::size_t &s) //will write BootstampLength chars
{
   if(s < (BootstampLength*2))
      return false;
   system_timeofday_information info;
   bool ret = get_system_time_of_day_information(info);
   if(!ret){
      return false;
   }
   const char Characters [] =
      { '0', '1', '2', '3', '4', '5', '6', '7'
      , '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
   std::size_t char_counter = 0;
   for(std::size_t i = 0; i != static_cast<std::size_t>(BootstampLength); ++i){
      bootstamp_str[char_counter++] = Characters[(info.Reserved1[i]&0xF0)>>4];
      bootstamp_str[char_counter++] = Characters[(info.Reserved1[i]&0x0F)];
   }
   s = BootstampLength*2;
   return true;
}

inline bool get_boot_and_system_time_wstr(wchar_t *bootsystemstamp, std::size_t &s)  //will write BootAndSystemstampLength chars
{
   if(s < (BootAndSystemstampLength*2))
      return false;
   system_timeofday_information info;
   bool ret = get_system_time_of_day_information(info);
   if(!ret){
      return false;
   }
   const wchar_t Characters [] =
      { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7'
      , L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };
   std::size_t char_counter = 0;
   for(std::size_t i = 0; i != static_cast<std::size_t>(BootAndSystemstampLength); ++i){
      bootsystemstamp[char_counter++] = Characters[(info.Reserved1[i]&0xF0)>>4];
      bootsystemstamp[char_counter++] = Characters[(info.Reserved1[i]&0x0F)];
   }
   s = BootAndSystemstampLength*2;
   return true;
}

class handle_closer
{
   void *handle_;
   public:
   handle_closer(void *handle) : handle_(handle){}
   ~handle_closer(){ close_handle(handle_);  }
};

union ntquery_mem_t
{
   object_name_information_t name;
   struct ren_t
   {
      file_rename_information_t info;
      wchar_t buf[32767];
   } ren;
};

inline bool unlink_file(const char *filename)
{
   try{
      NtSetInformationFile_t pNtSetInformationFile =
         (NtSetInformationFile_t)get_proc_address(get_module_handle("ntdll.dll"), "NtSetInformationFile"); 
      if(!pNtSetInformationFile){
         return false;
      }

      NtQueryObject_t pNtQueryObject =
         (NtQueryObject_t)get_proc_address(get_module_handle("ntdll.dll"), "NtQueryObject"); 

      //First step: Obtain a handle to the file using Win32 rules. This resolves relative paths
      void *fh = create_file(filename, generic_read | delete_access, open_existing,
         file_flag_backup_semantics | file_flag_delete_on_close); 
      if(fh == invalid_handle_value){
         return false;
      }

      handle_closer h_closer(fh);

      std::auto_ptr<ntquery_mem_t> pmem(new ntquery_mem_t);
      file_rename_information_t *pfri = (file_rename_information_t*)&pmem->ren.info;
      const std::size_t RenMaxNumChars =
         ((char*)pmem.get() - (char*)&pmem->ren.info.FileName[0])/sizeof(wchar_t);

      //Obtain file name
      unsigned long size;
      if(pNtQueryObject(fh, object_name_information, pmem.get(), sizeof(ntquery_mem_t), &size)){
         return false;
      }

      //Copy filename to the rename member
      std::memmove(pmem->ren.info.FileName, pmem->name.Name.Buffer, pmem->name.Name.Length);
      std::size_t filename_string_length = pmem->name.Name.Length/sizeof(wchar_t);

      //Second step: obtain the complete native-nt filename
      //if(!get_file_name_from_handle_function(fh, pfri->FileName, RenMaxNumChars, filename_string_length)){
      //return 0;
      //}

      //Add trailing mark
      if((RenMaxNumChars-filename_string_length) < (SystemTimeOfDayInfoLength*2)){
         return false;
      }

      //Search '\\' character to replace it
      for(std::size_t i = filename_string_length; i != 0; --filename_string_length){
         if(pmem->ren.info.FileName[--i] == L'\\')
            break;
      }

      //Add random number
      std::size_t s = RenMaxNumChars - filename_string_length;
      if(!get_boot_and_system_time_wstr(&pfri->FileName[filename_string_length], s)){
         return false;
      }
      filename_string_length += s;

      //Fill rename information (FileNameLength is in bytes)
      pfri->FileNameLength = static_cast<unsigned long>(sizeof(wchar_t)*(filename_string_length));
      pfri->Replace = 1;
      pfri->RootDir = 0;

      //Final step: change the name of the in-use file:
      io_status_block_t io;
      if(0 != pNtSetInformationFile(fh, &io, pfri, sizeof(ntquery_mem_t::ren_t), file_rename_information)){
         return false;
      }
      return true;
   }
   catch(...){
      return false;
   }
}

struct reg_closer
{
   RegCloseKey_t func_;
   void *key_;
   reg_closer(RegCloseKey_t func, void *key) : func_(func), key_(key){}
   ~reg_closer(){ (*func_)(key_);  }
};

inline void get_shared_documents_folder(std::string &s)
{
   s.clear();
   void *hAdvapi = load_library("Advapi32.dll");
   if (hAdvapi){
      library_unloader unloader(hAdvapi);
      //  Pointer to function RegOpenKeyA
      RegOpenKeyEx_t pRegOpenKey =
         (RegOpenKeyEx_t)get_proc_address(hAdvapi, "RegOpenKeyExA");
      if (pRegOpenKey){
         //  Pointer to function RegCloseKey
         RegCloseKey_t pRegCloseKey =
            (RegCloseKey_t)get_proc_address(hAdvapi, "RegCloseKey");
         if (pRegCloseKey){
            //  Pointer to function RegQueryValueA
            RegQueryValueEx_t pRegQueryValue =
               (RegQueryValueEx_t)get_proc_address(hAdvapi, "RegQueryValueExA");
            if (pRegQueryValue){
               //Open the key
               void *key;
               if ((*pRegOpenKey)( hkey_local_machine
                                 , "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"
                                 , 0
                                 , key_query_value
                                 , &key) == 0){
                  reg_closer key_closer(pRegCloseKey, key);

                  //Obtain the value
                  unsigned long size;
                  unsigned long type;
                  const char *const reg_value = "Common AppData";
                  long err = (*pRegQueryValue)( key, reg_value, 0, &type, 0, &size);
                  if(!err){
                     //Size includes terminating NULL
                     s.resize(size);
                     err = (*pRegQueryValue)( key, reg_value, 0, &type, (unsigned char*)(&s[0]), &size);
                     if(!err)
                        s.erase(s.end()-1);
                     (void)err;
                  }
               }
            }
         }
      }
   }
}

}  //namespace winapi 
}  //namespace interprocess
}  //namespace boost 

#include <boost/interprocess/detail/config_end.hpp>

#endif //#ifdef BOOST_INTERPROCESS_WIN32_SYNC_PRIMITIVES_HPP
