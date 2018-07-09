// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROCESS_DETAIL_WINDOWS_JOB_WORKAROUND_HPP_
#define BOOST_PROCESS_DETAIL_WINDOWS_JOB_WORKAROUND_HPP_

#include <boost/detail/winapi/config.hpp>
#include <boost/detail/winapi/basic_types.hpp>
#include <boost/detail/winapi/dll.hpp>

namespace boost { namespace process { namespace detail { namespace windows { namespace workaround {

//this import workaround is to keep it a header-only library. and enums cannot be imported from the winapi.

extern "C"
{

typedef enum _JOBOBJECTINFOCLASS_ {
      JobObjectBasicAccountingInformation_ = 1, JobObjectBasicLimitInformation_,
      JobObjectBasicProcessIdList_, JobObjectBasicUIRestrictions_,
      JobObjectSecurityLimitInformation_, JobObjectEndOfJobTimeInformation_,
      JobObjectAssociateCompletionPortInformation_, JobObjectBasicAndIoAccountingInformation_,
      JobObjectExtendedLimitInformation_, JobObjectJobSetInformation_,
      JobObjectGroupInformation_,
      JobObjectNotificationLimitInformation_,
      JobObjectLimitViolationInformation_,
      JobObjectGroupInformationEx_,
      JobObjectCpuRateControlInformation_,
      JobObjectCompletionFilter_,
      JobObjectCompletionCounter_,
      JobObjectReserved1Information_ = 18,
      JobObjectReserved2Information_,
      JobObjectReserved3Information_,
      JobObjectReserved4Information_,
      JobObjectReserved5Information_,
      JobObjectReserved6Information_,
      JobObjectReserved7Information_,
      JobObjectReserved8Information_,
      MaxJobObjectInfoClass_
    } JOBOBJECTINFOCLASS_;

typedef struct _JOBOBJECT_BASIC_LIMIT_INFORMATION_ {
  ::boost::detail::winapi::LARGE_INTEGER_ PerProcessUserTimeLimit;
  ::boost::detail::winapi::LARGE_INTEGER_ PerJobUserTimeLimit;
  ::boost::detail::winapi::DWORD_         LimitFlags;
  ::boost::detail::winapi::SIZE_T_        MinimumWorkingSetSize;
  ::boost::detail::winapi::SIZE_T_        MaximumWorkingSetSize;
  ::boost::detail::winapi::DWORD_         ActiveProcessLimit;
  ::boost::detail::winapi::ULONG_PTR_     Affinity;
  ::boost::detail::winapi::DWORD_         PriorityClass;
  ::boost::detail::winapi::DWORD_         SchedulingClass;
} JOBOBJECT_BASIC_LIMIT_INFORMATION_;


typedef struct _IO_COUNTERS_ {
  ::boost::detail::winapi::ULONGLONG_ ReadOperationCount;
  ::boost::detail::winapi::ULONGLONG_ WriteOperationCount;
  ::boost::detail::winapi::ULONGLONG_ OtherOperationCount;
  ::boost::detail::winapi::ULONGLONG_ ReadTransferCount;
  ::boost::detail::winapi::ULONGLONG_ WriteTransferCount;
  ::boost::detail::winapi::ULONGLONG_ OtherTransferCount;
} IO_COUNTERS_;


typedef struct _JOBOBJECT_EXTENDED_LIMIT_INFORMATION_ {
  JOBOBJECT_BASIC_LIMIT_INFORMATION_ BasicLimitInformation;
  IO_COUNTERS_                       IoInfo;
  ::boost::detail::winapi::SIZE_T_   ProcessMemoryLimit;
  ::boost::detail::winapi::SIZE_T_   JobMemoryLimit;
  ::boost::detail::winapi::SIZE_T_   PeakProcessMemoryUsed;
  ::boost::detail::winapi::SIZE_T_   PeakJobMemoryUsed;
} JOBOBJECT_EXTENDED_LIMIT_INFORMATION_;


/*BOOL WINAPI QueryInformationJobObject(
  _In_opt_  HANDLE             hJob,
  _In_      JOBOBJECTINFOCLASS JobObjectInfoClass,
  _Out_     LPVOID             lpJobObjectInfo,
  _In_      DWORD              cbJobObjectInfoLength,
  _Out_opt_ LPDWORD            lpReturnLength
);
 */
typedef ::boost::detail::winapi::BOOL_  ( WINAPI *query_information_job_object_p)(
        ::boost::detail::winapi::HANDLE_,
        JOBOBJECTINFOCLASS_,
        void *,
        ::boost::detail::winapi::DWORD_,
        ::boost::detail::winapi::DWORD_ *);


inline ::boost::detail::winapi::BOOL_ WINAPI query_information_job_object(
        ::boost::detail::winapi::HANDLE_ hJob,
        JOBOBJECTINFOCLASS_ JobObjectInfoClass,
        void * lpJobObjectInfo,
        ::boost::detail::winapi::DWORD_ cbJobObjectInfoLength,
        ::boost::detail::winapi::DWORD_ *lpReturnLength)
{
    static ::boost::detail::winapi::HMODULE_ h = ::boost::detail::winapi::get_module_handle("Kernel32.dll");
    static query_information_job_object_p f = reinterpret_cast<query_information_job_object_p>(::boost::detail::winapi::get_proc_address(h, "QueryInformationJobObject"));

    return (*f)(hJob, JobObjectInfoClass, lpJobObjectInfo, cbJobObjectInfoLength, lpReturnLength);
}

/*BOOL WINAPI SetInformationJobObject(
  _In_ HANDLE             hJob,
  _In_ JOBOBJECTINFOCLASS JobObjectInfoClass,
  _In_ LPVOID             lpJobObjectInfo,
  _In_ DWORD              cbJobObjectInfoLength
);*/

typedef ::boost::detail::winapi::BOOL_  ( WINAPI *set_information_job_object_p)(
        ::boost::detail::winapi::HANDLE_,
        JOBOBJECTINFOCLASS_,
        void *,
        ::boost::detail::winapi::DWORD_);

}

inline ::boost::detail::winapi::BOOL_ WINAPI set_information_job_object(
        ::boost::detail::winapi::HANDLE_ hJob,
        JOBOBJECTINFOCLASS_ JobObjectInfoClass,
        void * lpJobObjectInfo,
        ::boost::detail::winapi::DWORD_ cbJobObjectInfoLength)
{
    static ::boost::detail::winapi::HMODULE_ h = ::boost::detail::winapi::get_module_handle("Kernel32.dll");
    static set_information_job_object_p f = reinterpret_cast<set_information_job_object_p>(::boost::detail::winapi::get_proc_address(h, "SetInformationJobObject"));

    return (*f)(hJob, JobObjectInfoClass, lpJobObjectInfo, cbJobObjectInfoLength);
}

constexpr static ::boost::detail::winapi::DWORD_ JOB_OBJECT_LIMIT_BREAKAWAY_OK_ = 0x00000800;

}}}}}



#endif /* BOOST_PROCESS_DETAIL_WINDOWS_JOB_WORKAROUND_HPP_ */
