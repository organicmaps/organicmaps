#pragma once
#ifndef DSSI_EXPATIMPL_H
#define DSSI_EXPATIMPL_H

#include <cstring>


//-----------------------------------------------------------------------------
//
// @doc
//
// @module	ExpatImpl.h - Expat class container |
//
// This module contains the definition of the expat class container.
//
// Copyright (c) 1994-2002 - Descartes Systems Sciences, Inc.
//
// @end
//
// $History: ExpatImpl.h $
//
//      *****************  Version 1  *****************
//      User: Tim Smith    Date: 1/29/02    Time: 1:57p
//      Created in $/Omni_V2/_ToolLib
//      1. String.h now replaced with StringCode.h.
//      2. StringRsrc.h modified to use new string class.
//      3. Added tons of new classes from the wedge work.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// Required include files
//
//-----------------------------------------------------------------------------

#include <assert.h>
#include "lib/expat.h"

//-----------------------------------------------------------------------------
//
// Forward definitions
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// Template class definition
//
//-----------------------------------------------------------------------------

template <class _T>
class CExpatImpl
{

// @access Constructors and destructors
public:

  // @cmember General constructor

  CExpatImpl ()
  {
    m_p = NULL;
  }

  // @cmember Destructor

  ~CExpatImpl ()
  {
    Destroy ();
  }

// @access Parser creation and deletion methods
public:

  // @cmember Create a parser

  bool Create (const XML_Char *pszEncoding = NULL,
    const XML_Char *pszSep = NULL)
  {

    //
    // Destroy the old parser
    //

    Destroy ();

    //
    // If the encoding or seperator are empty, then NULL
    //

    if (pszEncoding != NULL && pszEncoding [0] == 0)
      pszEncoding = NULL;
    if (pszSep != NULL && pszSep [0] == 0)
      pszSep = NULL;

    //
    // Create the new one
    //

    m_p = XML_ParserCreate_MM (pszEncoding, NULL, pszSep);
    if (m_p == NULL)
      return false;

    //
    // Invoke the post create routine
    //

    _T *pThis = static_cast <_T *> (this);
    pThis ->OnPostCreate ();

    //
    // Set the user data used in callbacks
    //

    XML_SetUserData (m_p, (void *) this);
    return true;
  }

  // @cmember Destroy the parser

  void Destroy ()
  {
    if (m_p != NULL)
      XML_ParserFree (m_p);
    m_p = NULL;
  }

// @access Parser parse methods
public:

  // @cmember Parse a block of data

  bool Parse (const char *pszBuffer, int nLength = -1, bool fIsFinal = true)
  {

    //
    // Validate
    //

    assert (m_p != NULL);

    //
    // Get the length if not specified
    //

    if (nLength < 0)
      nLength = strlen (pszBuffer);

    //
    // Invoke the parser
    //

    return XML_Parse (m_p, pszBuffer, nLength, fIsFinal) != 0;
  }

  // @cmember Parse a block of data

#ifdef WCHAR
  bool Parse (const WCHAR *pszBuffer, int nLength = -1, bool fIsFinal = true)
  {

    //
    // Validate
    //

    assert (m_p != NULL);

    //
    // Get the length if not specified
    //

    if (nLength < 0)
      nLength = wcslen (pszBuffer) * 2;

    //
    // Invoke the parser
    //

    return XML_Parse (m_p, pszBuffer, nLength, fIsFinal) != 0;
  }
#endif

  // @cmember Parse internal buffer

  bool ParseBuffer (int nLength, bool fIsFinal = true)
  {
    assert (m_p != NULL);
    return XML_ParseBuffer (m_p, nLength, fIsFinal) != 0;
  }

  // @cmember Get the internal buffer

  void *GetBuffer (int nLength)
  {
    assert (m_p != NULL);
    return XML_GetBuffer (m_p, nLength);
  }

// @access Parser callback enable/disable methods
public:

  // @cmember Enable/Disable the start element handler

  void EnableStartElementHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetStartElementHandler (m_p, fEnable ? StartElementHandler : NULL);
  }

  // @cmember Enable/Disable the end element handler

  void EnableEndElementHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetEndElementHandler (m_p, fEnable ? EndElementHandler : NULL);
  }

  // @cmember Enable/Disable the element handlers

  void EnableElementHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    EnableStartElementHandler (fEnable);
    EnableEndElementHandler (fEnable);
  }

  // @cmember Enable/Disable the character data handler

  void EnableCharacterDataHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetCharacterDataHandler (m_p,
      fEnable ? CharacterDataHandler : NULL);
  }

  // @cmember Enable/Disable the processing instruction handler

  void EnableProcessingInstructionHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetProcessingInstructionHandler (m_p,
      fEnable ? ProcessingInstructionHandler : NULL);
  }

  // @cmember Enable/Disable the comment handler

  void EnableCommentHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetCommentHandler (m_p, fEnable ? CommentHandler : NULL);
  }

  // @cmember Enable/Disable the start CDATA section handler

  void EnableStartCdataSectionHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetStartCdataSectionHandler (m_p,
      fEnable ? StartCdataSectionHandler : NULL);
  }

  // @cmember Enable/Disable the end CDATA section handler

  void EnableEndCdataSectionHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetEndCdataSectionHandler (m_p,
      fEnable ? EndCdataSectionHandler : NULL);
  }

  // @cmember Enable/Disable the CDATA section handlers

  void EnableCdataSectionHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    EnableStartCdataSectionHandler (fEnable);
    EnableEndCdataSectionHandler (fEnable);
  }

  // @cmember Enable/Disable default handler

  void EnableDefaultHandler (bool fEnable = true, bool fExpand = true)
  {
    assert (m_p != NULL);
    if (fExpand)
    {
      XML_SetDefaultHandlerExpand (m_p,
        fEnable ? DefaultHandler : NULL);
    }
    else
      XML_SetDefaultHandler (m_p, fEnable ? DefaultHandler : NULL);
  }

  // @cmember Enable/Disable external entity ref handler

  void EnableExternalEntityRefHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetExternalEntityRefHandler (m_p,
      fEnable ? ExternalEntityRefHandler : NULL);
  }

  // @cmember Enable/Disable unknown encoding handler

// disabled because it causes compile errors with gcc 7
#if 0
  void EnableUnknownEncodingHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetUnknownEncodingHandler (m_p,
      fEnable ? UnknownEncodingHandler : NULL);
  }
#endif

  // @cmember Enable/Disable start namespace handler

  void EnableStartNamespaceDeclHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetStartNamespaceDeclHandler (m_p,
      fEnable ? StartNamespaceDeclHandler : NULL);
  }

  // @cmember Enable/Disable end namespace handler

  void EnableEndNamespaceDeclHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetEndNamespaceDeclHandler (m_p,
      fEnable ? EndNamespaceDeclHandler : NULL);
  }

  // @cmember Enable/Disable namespace handlers

  void EnableNamespaceDeclHandler (bool fEnable = true)
  {
    EnableStartNamespaceDeclHandler (fEnable);
    EnableEndNamespaceDeclHandler (fEnable);
  }

  // @cmember Enable/Disable the XML declaration handler

  void EnableXmlDeclHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetXmlDeclHandler (m_p, fEnable ? XmlDeclHandler : NULL);
  }

  // @cmember Enable/Disable the start DOCTYPE declaration handler

  void EnableStartDoctypeDeclHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetStartDoctypeDeclHandler (m_p,
      fEnable ? StartDoctypeDeclHandler : NULL);
  }

  // @cmember Enable/Disable the end DOCTYPE declaration handler

  void EnableEndDoctypeDeclHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    XML_SetEndDoctypeDeclHandler (m_p,
      fEnable ? EndDoctypeDeclHandler : NULL);
  }

  // @cmember Enable/Disable the DOCTYPE declaration handler

  void EnableDoctypeDeclHandler (bool fEnable = true)
  {
    assert (m_p != NULL);
    EnableStartDoctypeDeclHandler (fEnable);
    EnableEndDoctypeDeclHandler (fEnable);
  }

// @access Parser error reporting methods
public:

  // @cmember Get last error

  enum XML_Error GetErrorCode ()
  {
    assert (m_p != NULL);
    return XML_GetErrorCode (m_p);
  }

  // @cmember Get the current byte index

  long GetCurrentByteIndex ()
  {
    assert (m_p != NULL);
    return XML_GetCurrentByteIndex (m_p);
  }

  // @cmember Get the current line number

  unsigned long GetCurrentLineNumber ()
  {
    assert (m_p != NULL);
    return XML_GetCurrentLineNumber (m_p);
  }

  // @cmember Get the current column number

  int GetCurrentColumnNumber ()
  {
    assert (m_p != NULL);
    return XML_GetCurrentColumnNumber (m_p);
  }

  // @cmember Get the current byte count

  int GetCurrentByteCount ()
  {
    assert (m_p != NULL);
    return XML_GetCurrentByteCount (m_p);
  }

  // @cmember Get the input context

  const char *GetInputContext (int *pnOffset, int *pnSize)
  {
    assert (m_p != NULL);
    return XML_GetInputContext (m_p, pnOffset, pnSize);
  }

  // @cmember Get last error string

  const XML_LChar *GetErrorString ()
  {
    return XML_ErrorString (GetErrorCode ());
  }

// @access Parser other methods
public:

  // @cmember Return the version string

  static const XML_LChar *GetExpatVersion ()
  {
    return XML_ExpatVersion ();
  }

  // @cmember Get the version information

/*
  static void GetExpatVersion (int *pnMajor, int *pnMinor, int *pnMicro)
  {
    XML_expat_version v = XML_ExpatVersionInfo ();
    if (pnMajor)
      *pnMajor = v .major;
    if (pnMinor)
      *pnMinor = v .minor;
    if (pnMicro)
      *pnMicro = v .micro;
  }
*/

  // @cmember Get last error string

  static const XML_LChar *GetErrorString (enum XML_Error nError)
  {
    return XML_ErrorString (nError);
  }

// @access Public handler methods
public:

  // @cmember Start element handler

  void OnStartElement (const XML_Char *pszName, const XML_Char **papszAttrs)
  {
    return;
  }

  // @cmember End element handler

  void OnEndElement (const XML_Char *pszName)
  {
    return;
  }

  // @cmember Character data handler

  void OnCharacterData (const XML_Char *pszData, int nLength)
  {
    return;
  }

  // @cmember Processing instruction handler

  void OnProcessingInstruction (const XML_Char *pszTarget,
    const XML_Char *pszData)
  {
    return;
  }

  // @cmember Comment handler

  void OnComment (const XML_Char *pszData)
  {
    return;
  }

  // @cmember Start CDATA section handler

  void OnStartCdataSection ()
  {
    return;
  }

  // @cmember End CDATA section handler

  void OnEndCdataSection ()
  {
    return;
  }

  // @cmember Default handler

  void OnDefault (const XML_Char *pszData, int nLength)
  {
    return;
  }

  // @cmember External entity ref handler

  bool OnExternalEntityRef (const XML_Char *pszContext,
    const XML_Char *pszBase, const XML_Char *pszSystemID,
    const XML_Char *pszPublicID)
  {
    return false;
  }

  // @cmember Unknown encoding handler

  bool OnUnknownEncoding (const XML_Char *pszName, XML_Encoding *pInfo)
  {
    return false;
  }

  // @cmember Start namespace declaration handler

  void OnStartNamespaceDecl (const XML_Char *pszPrefix,
    const XML_Char *pszURI)
  {
    return;
  }

  // @cmember End namespace declaration handler

  void OnEndNamespaceDecl (const XML_Char *pszPrefix)
  {
    return;
  }

  // @cmember XML declaration handler

  void OnXmlDecl (const XML_Char *pszVersion, const XML_Char *pszEncoding,
    bool fStandalone)
  {
    return;
  }

  // @cmember Start DOCTYPE declaration handler

  void OnStartDoctypeDecl (const XML_Char *pszDoctypeName,
    const XML_Char *pszSysID, const XML_Char *pszPubID,
    bool fHasInternalSubset)
  {
    return;
  }

  // @cmember End DOCTYPE declaration handler

  void OnEndDoctypeDecl ()
  {
    return;
  }

// @access Protected methods
protected:

  // @cmember Handle any post creation

  void OnPostCreate ()
  {
  }

// @access Protected static methods
protected:

  // @cmember Start element handler wrapper

  static void StartElementHandler (void *pUserData,
    const XML_Char *pszName, const XML_Char **papszAttrs)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    pThis ->OnStartElement (pszName, papszAttrs);
  }

  // @cmember End element handler wrapper

  static void EndElementHandler (void *pUserData,
    const XML_Char *pszName)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    pThis ->OnEndElement (pszName);
  }

  // @cmember Character data handler wrapper

  static void CharacterDataHandler (void *pUserData,
    const XML_Char *pszData, int nLength)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    pThis ->OnCharacterData (pszData, nLength);
  }

  // @cmember Processing instruction handler wrapper

  static void ProcessingInstructionHandler (void *pUserData,
    const XML_Char *pszTarget, const XML_Char *pszData)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    pThis ->OnProcessingInstruction (pszTarget, pszData);
  }

  // @cmember Comment handler wrapper

  static void CommentHandler (void *pUserData,
    const XML_Char *pszData)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    pThis ->OnComment (pszData);
  }

  // @cmember Start CDATA section wrapper

  static void StartCdataSectionHandler (void *pUserData)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    pThis ->OnStartCdataSection ();
  }

  // @cmember End CDATA section wrapper

  static void EndCdataSectionHandler (void *pUserData)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    pThis ->OnEndCdataSection ();
  }

  // @cmember Default wrapper

  static void DefaultHandler (void *pUserData,
    const XML_Char *pszData, int nLength)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    pThis ->OnDefault (pszData, nLength);
  }

  // @cmember External entity ref wrapper

  static int ExternalEntityRefHandler (void *pUserData,
    const XML_Char *pszContext, const XML_Char *pszBase,
    const XML_Char *pszSystemID, const XML_Char *pszPublicID)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    return pThis ->OnExternalEntityRef (pszContext,
      pszBase, pszSystemID, pszPublicID) ? 1 : 0;
  }

  // @cmember Unknown encoding wrapper

  static int UnknownEncodingHandler (void *pUserData,
    const XML_Char *pszName, XML_Encoding *pInfo)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    return pThis ->OnUnknownEncoding (pszName, pInfo) ? 1 : 0;
  }

  // @cmember Start namespace decl wrapper

  static void StartNamespaceDeclHandler (void *pUserData,
    const XML_Char *pszPrefix, const XML_Char *pszURI)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    pThis ->OnStartNamespaceDecl (pszPrefix, pszURI);
  }

  // @cmember End namespace decl wrapper

  static void EndNamespaceDeclHandler (void *pUserData,
    const XML_Char *pszPrefix)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    pThis ->OnEndNamespaceDecl (pszPrefix);
  }

  // @cmember XML declaration wrapper

  static void XmlDeclHandler (void *pUserData,
    const XML_Char *pszVersion, const XML_Char *pszEncoding,
    int nStandalone)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    pThis ->OnXmlDecl (pszVersion, pszEncoding, nStandalone != 0);
  }

  // @cmember Start Doctype declaration wrapper

  static void StartDoctypeDeclHandler (void *pUserData,
    const XML_Char *pszDoctypeName, const XML_Char *pszSysID,
    const XML_Char *pszPubID, int nHasInternalSubset)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    pThis ->OnStartDoctypeDecl (pszDoctypeName, pszSysID,
      pszPubID, nHasInternalSubset != 0);
  }

  // @cmember End Doctype declaration wrapper

  static void EndDoctypeDeclHandler (void *pUserData)
  {
    _T *pThis = static_cast <_T *> ((CExpatImpl <_T> *) pUserData);
    pThis ->OnEndDoctypeDecl ();
  }

// @access Protected members
protected:

  XML_Parser		m_p;
};

//-----------------------------------------------------------------------------
//
// Virtual method class definition
//
//-----------------------------------------------------------------------------

class CExpat : public CExpatImpl <CExpat>
{
// @access Constructors and destructors
public:

  CExpat ()
  {
  }

  virtual ~CExpat() {}

// @access Public handler methods
public:

  // @cmember Start element handler

  virtual void OnStartElement (const XML_Char * /*pszName*/,
    const XML_Char ** /*papszAttrs*/)
  {
    return;
  }

  // @cmember End element handler

  virtual void OnEndElement (const XML_Char * /*pszName*/)
  {
    return;
  }

  // @cmember Character data handler

  virtual void OnCharacterData (const XML_Char * /*pszData*/, int /*nLength*/)
  {
    return;
  }

  // @cmember Processing instruction handler

  virtual void OnProcessingInstruction (const XML_Char * /*pszTarget*/,
    const XML_Char * /*pszData*/)
  {
    return;
  }

  // @cmember Comment handler

  virtual void OnComment (const XML_Char * /*pszData*/)
  {
    return;
  }

  // @cmember Start CDATA section handler

  virtual void OnStartCdataSection ()
  {
    return;
  }

  // @cmember End CDATA section handler

  virtual void OnEndCdataSection ()
  {
    return;
  }

  // @cmember Default handler

  virtual void OnDefault (const XML_Char * /*pszData*/, int /*nLength*/)
  {
    return;
  }

  // @cmember External entity ref handler

  virtual bool OnExternalEntityRef (const XML_Char * /*pszContext*/,
    const XML_Char * /*pszBase*/, const XML_Char * /*pszSystemID*/,
    const XML_Char * /*pszPublicID*/)
  {
    return false;
  }

  // @cmember Unknown encoding handler

  virtual bool OnUnknownEncoding (const XML_Char * /*pszName*/, XML_Encoding * /*pInfo*/)
  {
    return false;
  }

  // @cmember Start namespace declaration handler

  virtual void OnStartNamespaceDecl (const XML_Char * /*pszPrefix*/,
    const XML_Char * /*pszURI*/)
  {
    return;
  }

  // @cmember End namespace declaration handler

  virtual void OnEndNamespaceDecl (const XML_Char * /*pszPrefix*/)
  {
    return;
  }

  // @cmember XML declaration handler

  virtual void OnXmlDecl (const XML_Char * /*pszVersion*/,
    const XML_Char * /*pszEncoding*/, bool /*fStandalone*/)
  {
    return;
  }

  // @cmember Start DOCTYPE declaration handler

  virtual void OnStartDoctypeDecl (const XML_Char * /*pszDoctypeName*/,
    const XML_Char * /*pszSysID*/, const XML_Char * /*pszPubID*/,
    bool /*fHasInternalSubset*/)
  {
    return;
  }

  // @cmember End DOCTYPE declaration handler

  virtual void OnEndDoctypeDecl ()
  {
    return;
  }
};

#endif // DSSI_EXPATIMPL_H
