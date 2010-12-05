/*
** Copyright (c) 2001-2007 Expat maintainers.
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
** 
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <dos/dos.h>
#include <proto/exec.h>

#define LIBNAME		"expat.library"
#define LIBPRI		0
#define VERSION		4
#define REVISION	2
#define VSTRING		"expat.library 4.2 (2.6.2007)"  /* dd.mm.yyyy */


static const char* __attribute__((used)) verstag = "\0$VER: " VSTRING;


struct ExpatBase {
	struct Library libNode;
	uint16 pad;
	BPTR SegList;
};


struct ExpatBase * libInit(struct ExpatBase *libBase, BPTR seglist, struct ExecIFace *ISys);
uint32 libObtain (struct LibraryManagerInterface *Self);
uint32 libRelease (struct LibraryManagerInterface *Self);
struct ExpatBase *libOpen (struct LibraryManagerInterface *Self, uint32 version);
BPTR libClose (struct LibraryManagerInterface *Self);
BPTR libExpunge (struct LibraryManagerInterface *Self);


static APTR lib_manager_vectors[] = {
	libObtain,
	libRelease,
	NULL,
	NULL,
	libOpen,
	libClose,
	libExpunge,
	NULL,
	(APTR)-1,
};


static struct TagItem lib_managerTags[] = {
	{ MIT_Name, (uint32)"__library" },
	{ MIT_VectorTable, (uint32)lib_manager_vectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};


extern void *main_vectors[];

static struct TagItem lib_mainTags[] = {
	{ MIT_Name, (uint32)"main" },
	{ MIT_VectorTable, (uint32)main_vectors },
	{ MIT_Version, 1 },
	{ TAG_END, 0 }
};


static APTR libInterfaces[] = {
	lib_managerTags,
	lib_mainTags,
	NULL
};


static struct TagItem libCreateTags[] = {
	{ CLT_DataSize, sizeof(struct ExpatBase) },
	{ CLT_InitFunc, (uint32)libInit },
	{ CLT_Interfaces, (uint32)libInterfaces },
	{ TAG_END, 0 }
};


static struct Resident __attribute__((used)) lib_res = {
	RTC_MATCHWORD,	// rt_MatchWord
	&lib_res,		// rt_MatchTag
	&lib_res+1,		// rt_EndSkip
	RTF_NATIVE | RTF_AUTOINIT,	// rt_Flags
	VERSION,		// rt_Version
	NT_LIBRARY,		// rt_Type
	LIBPRI,			// rt_Pri
	LIBNAME,		// rt_Name
	VSTRING,		// rt_IdString
	libCreateTags	// rt_Init
};


struct Library *DOSLib = 0;
struct Library *UtilityBase = 0;

struct ExecIFace *IExec = 0;
struct DOSIFace *IDOS = 0;
struct UtilityIFace *IUtility = 0;


void _start()
{
}


struct ExpatBase *libInit(struct ExpatBase *libBase, BPTR seglist, struct ExecIFace *ISys)
{
	libBase->libNode.lib_Node.ln_Type = NT_LIBRARY;
	libBase->libNode.lib_Node.ln_Pri = LIBPRI;
	libBase->libNode.lib_Node.ln_Name = LIBNAME;
	libBase->libNode.lib_Flags = LIBF_SUMUSED|LIBF_CHANGED;
	libBase->libNode.lib_Version = VERSION;
	libBase->libNode.lib_Revision = REVISION;
	libBase->libNode.lib_IdString = VSTRING;
	libBase->SegList = seglist;

	IExec = ISys;

	DOSLib = OpenLibrary("dos.library", 51);
	if ( DOSLib != 0 )  {
		IDOS = (struct DOSIFace *)GetInterface(DOSLib, "main", 1, NULL);
		if ( IDOS != 0 )  {
			UtilityBase = OpenLibrary("utility.library", 51);
			if ( UtilityBase != 0 )  {
				IUtility = (struct UtilityIFace*)GetInterface(UtilityBase, "main", 1, NULL);
				if ( IUtility != 0 )  {
					return libBase;
				}

				CloseLibrary(UtilityBase);
			}

			DropInterface((struct Interface *)IDOS);
		}

		CloseLibrary(DOSLib);
	}

	return NULL;
}


uint32 libObtain( struct LibraryManagerInterface *Self )
{
	++Self->Data.RefCount;
	return Self->Data.RefCount;
}


uint32 libRelease( struct LibraryManagerInterface *Self )
{
	--Self->Data.RefCount;
	return Self->Data.RefCount;
}


struct ExpatBase *libOpen( struct LibraryManagerInterface *Self, uint32 version )
{
	struct ExpatBase *libBase;

	libBase = (struct ExpatBase *)Self->Data.LibBase;

	++libBase->libNode.lib_OpenCnt;
	libBase->libNode.lib_Flags &= ~LIBF_DELEXP;

	return libBase;
}


BPTR libClose( struct LibraryManagerInterface *Self )
{
	struct ExpatBase *libBase;

	libBase = (struct ExpatBase *)Self->Data.LibBase;

	--libBase->libNode.lib_OpenCnt;
	if ( libBase->libNode.lib_OpenCnt ) {
		return 0;
	}

	if ( libBase->libNode.lib_Flags & LIBF_DELEXP ) {
		return (BPTR)Self->LibExpunge();
	}
	else {
		return 0;
	}
}


BPTR libExpunge( struct LibraryManagerInterface *Self )
{
	struct ExpatBase *libBase;
	BPTR result = 0;

	libBase = (struct ExpatBase *)Self->Data.LibBase;

	if (libBase->libNode.lib_OpenCnt == 0) {
		Remove(&libBase->libNode.lib_Node);

		result = libBase->SegList;

		DropInterface((struct Interface *)IUtility);
		CloseLibrary(UtilityBase);
		DropInterface((struct Interface *)IDOS);
		CloseLibrary(DOSLib);

		DeleteLibrary(&libBase->libNode);
	}
	else {
		libBase->libNode.lib_Flags |= LIBF_DELEXP;
	}

	return result;
}
