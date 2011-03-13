# Microsoft Developer Studio Project File - Name="libtomcrypt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libtomcrypt - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libtomcrypt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libtomcrypt.mak" CFG="libtomcrypt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libtomcrypt - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libtomcrypt - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "libtomcrypt"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libtomcrypt - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "src\headers" /I "..\libtommath" /D "NDEBUG" /D "LTM_DESC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "LTC_SOURCE" /D "USE_LTM" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\tomcrypt.lib"

!ELSEIF  "$(CFG)" == "libtomcrypt - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "src\headers" /I "..\libtommath" /D "_DEBUG" /D "LTM_DESC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "LTC_SOURCE" /D "USE_LTM" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\tomcrypt.lib"

!ENDIF 

# Begin Target

# Name "libtomcrypt - Win32 Release"
# Name "libtomcrypt - Win32 Debug"
# Begin Group "ciphers"

# PROP Default_Filter ""
# Begin Group "aes"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\ciphers\aes\aes.c

!IF  "$(CFG)" == "libtomcrypt - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\src\ciphers\aes\aes.c

BuildCmds= \
	cl /nologo /MLd /W3 /Gm /GX /ZI /Od /I "src\headers" /I "..\libtommath" /D "_DEBUG" /D "LTM_DESC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "LTC_SOURCE" /D "USE_LTM" /Fp"Release/libtomcrypt.pch" /YX /Fo"Release/" /Fd"Release/" /FD /GZ /c $(InputPath) \
	cl /nologo /DENCRYPT_ONLY /MLd /W3 /Gm /GX /ZI /Od /I "src\headers" /I "..\libtommath" /D "_DEBUG" /D "LTM_DESC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "LTC_SOURCE" /D "USE_LTM" /Fp"Release/libtomcrypt.pch" /YX /Fo"Release/aes_enc.obj" /Fd"Release/" /FD /GZ /c $(InputPath) \
	

"Release/aes.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"Release/aes_enc.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "libtomcrypt - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\src\ciphers\aes\aes.c

BuildCmds= \
	cl /nologo /MLd /W3 /Gm /GX /ZI /Od /I "src\headers" /I "..\libtommath" /D "_DEBUG" /D "LTM_DESC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "LTC_SOURCE" /D "USE_LTM" /Fp"Debug/libtomcrypt.pch" /YX /Fo"Debug/" /Fd"Debug/" /FD /GZ /c $(InputPath) \
	cl /nologo /DENCRYPT_ONLY /MLd /W3 /Gm /GX /ZI /Od /I "src\headers" /I "..\libtommath" /D "_DEBUG" /D "LTM_DESC" /D "WIN32" /D "_MBCS" /D "_LIB" /D "LTC_SOURCE" /D "USE_LTM" /Fp"Debug/libtomcrypt.pch" /YX /Fo"Debug/aes_enc.obj" /Fd"Debug/" /FD /GZ /c $(InputPath) \
	

"Debug/aes.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"Debug/aes_enc.obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\ciphers\aes\aes_tab.c
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "safer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\ciphers\safer\safer.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\safer\safer_tab.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\safer\saferp.c
# End Source File
# End Group
# Begin Group "twofish"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\ciphers\twofish\twofish.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\twofish\twofish_tab.c
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\ciphers\anubis.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\blowfish.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\cast5.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\des.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\kasumi.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\khazad.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\kseed.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\multi2.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\noekeon.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\rc2.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\rc5.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\rc6.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\skipjack.c
# End Source File
# Begin Source File

SOURCE=.\src\ciphers\xtea.c
# End Source File
# End Group
# Begin Group "encauth"

# PROP Default_Filter ""
# Begin Group "ccm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\encauth\ccm\ccm_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\ccm\ccm_test.c
# End Source File
# End Group
# Begin Group "eax"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\encauth\eax\eax_addheader.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\eax\eax_decrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\eax\eax_decrypt_verify_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\eax\eax_done.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\eax\eax_encrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\eax\eax_encrypt_authenticate_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\eax\eax_init.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\eax\eax_test.c
# End Source File
# End Group
# Begin Group "gcm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\encauth\gcm\gcm_add_aad.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\gcm\gcm_add_iv.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\gcm\gcm_done.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\gcm\gcm_gf_mult.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\gcm\gcm_init.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\gcm\gcm_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\gcm\gcm_mult_h.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\gcm\gcm_process.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\gcm\gcm_reset.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\gcm\gcm_test.c
# End Source File
# End Group
# Begin Group "ocb"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\encauth\ocb\ocb_decrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\ocb\ocb_decrypt_verify_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\ocb\ocb_done_decrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\ocb\ocb_done_encrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\ocb\ocb_encrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\ocb\ocb_encrypt_authenticate_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\ocb\ocb_init.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\ocb\ocb_ntz.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\ocb\ocb_shift_xor.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\ocb\ocb_test.c
# End Source File
# Begin Source File

SOURCE=.\src\encauth\ocb\s_ocb_done.c
# End Source File
# End Group
# End Group
# Begin Group "hashes"

# PROP Default_Filter ""
# Begin Group "helper"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\hashes\helper\hash_file.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\helper\hash_filehandle.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\helper\hash_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\helper\hash_memory_multi.c
# End Source File
# End Group
# Begin Group "sha2"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\hashes\sha2\sha224.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\hashes\sha2\sha256.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\sha2\sha384.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\hashes\sha2\sha512.c
# End Source File
# End Group
# Begin Group "whirl"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\hashes\whirl\whirl.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\whirl\whirltab.c
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\hashes\chc\chc.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\md2.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\md4.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\md5.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\rmd128.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\rmd160.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\rmd256.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\rmd320.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\sha1.c
# End Source File
# Begin Source File

SOURCE=.\src\hashes\tiger.c
# End Source File
# End Group
# Begin Group "mac"

# PROP Default_Filter ""
# Begin Group "f9"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\mac\f9\f9_done.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\f9\f9_file.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\f9\f9_init.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\f9\f9_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\f9\f9_memory_multi.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\f9\f9_process.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\f9\f9_test.c
# End Source File
# End Group
# Begin Group "hmac"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\mac\hmac\hmac_done.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\hmac\hmac_file.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\hmac\hmac_init.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\hmac\hmac_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\hmac\hmac_memory_multi.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\hmac\hmac_process.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\hmac\hmac_test.c
# End Source File
# End Group
# Begin Group "omac"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\mac\omac\omac_done.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\omac\omac_file.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\omac\omac_init.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\omac\omac_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\omac\omac_memory_multi.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\omac\omac_process.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\omac\omac_test.c
# End Source File
# End Group
# Begin Group "pelican"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\mac\pelican\pelican.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\pelican\pelican_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\pelican\pelican_test.c
# End Source File
# End Group
# Begin Group "pmac"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\mac\pmac\pmac_done.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\pmac\pmac_file.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\pmac\pmac_init.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\pmac\pmac_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\pmac\pmac_memory_multi.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\pmac\pmac_ntz.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\pmac\pmac_process.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\pmac\pmac_shift_xor.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\pmac\pmac_test.c
# End Source File
# End Group
# Begin Group "xcbc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\mac\xcbc\xcbc_done.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\xcbc\xcbc_file.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\xcbc\xcbc_init.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\xcbc\xcbc_memory.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\xcbc\xcbc_memory_multi.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\xcbc\xcbc_process.c
# End Source File
# Begin Source File

SOURCE=.\src\mac\xcbc\xcbc_test.c
# End Source File
# End Group
# End Group
# Begin Group "math"

# PROP Default_Filter ""
# Begin Group "fp"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\math\fp\ltc_ecc_fp_mulmod.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\math\gmp_desc.c
# End Source File
# Begin Source File

SOURCE=.\src\math\ltm_desc.c
# End Source File
# Begin Source File

SOURCE=.\src\math\multi.c
# End Source File
# Begin Source File

SOURCE=.\src\math\rand_prime.c
# End Source File
# Begin Source File

SOURCE=.\src\math\tfm_desc.c
# End Source File
# End Group
# Begin Group "misc"

# PROP Default_Filter ""
# Begin Group "base64"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\misc\base64\base64_decode.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\base64\base64_encode.c
# End Source File
# End Group
# Begin Group "crypt"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\misc\crypt\crypt.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_argchk.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_cipher_descriptor.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_cipher_is_valid.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_find_cipher.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_find_cipher_any.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_find_cipher_id.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_find_hash.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_find_hash_any.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_find_hash_id.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_find_hash_oid.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_find_prng.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_fsa.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_hash_descriptor.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_hash_is_valid.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_ltc_mp_descriptor.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_prng_descriptor.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_prng_is_valid.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_register_cipher.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_register_hash.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_register_prng.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_unregister_cipher.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_unregister_hash.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\crypt\crypt_unregister_prng.c
# End Source File
# End Group
# Begin Group "pkcs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\misc\pkcs5\pkcs_5_1.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\pkcs5\pkcs_5_2.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\misc\burn_stack.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\error_to_string.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\zeromem.c
# End Source File
# End Group
# Begin Group "modes"

# PROP Default_Filter ""
# Begin Group "cbc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\modes\cbc\cbc_decrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\cbc\cbc_done.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\cbc\cbc_encrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\cbc\cbc_getiv.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\cbc\cbc_setiv.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\cbc\cbc_start.c
# End Source File
# End Group
# Begin Group "cfb"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\modes\cfb\cfb_decrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\cfb\cfb_done.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\cfb\cfb_encrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\cfb\cfb_getiv.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\cfb\cfb_setiv.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\cfb\cfb_start.c
# End Source File
# End Group
# Begin Group "ctr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\modes\ctr\ctr_decrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ctr\ctr_done.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ctr\ctr_encrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ctr\ctr_getiv.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ctr\ctr_setiv.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ctr\ctr_start.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ctr\ctr_test.c
# End Source File
# End Group
# Begin Group "ecb"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\modes\ecb\ecb_decrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ecb\ecb_done.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ecb\ecb_encrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ecb\ecb_start.c
# End Source File
# End Group
# Begin Group "f8"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\modes\f8\f8_decrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\f8\f8_done.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\f8\f8_encrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\f8\f8_getiv.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\f8\f8_setiv.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\f8\f8_start.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\f8\f8_test_mode.c
# End Source File
# End Group
# Begin Group "lrw"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\modes\lrw\lrw_decrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\lrw\lrw_done.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\lrw\lrw_encrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\lrw\lrw_getiv.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\lrw\lrw_process.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\lrw\lrw_setiv.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\lrw\lrw_start.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\lrw\lrw_test.c
# End Source File
# End Group
# Begin Group "ofb"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\modes\ofb\ofb_decrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ofb\ofb_done.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ofb\ofb_encrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ofb\ofb_getiv.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ofb\ofb_setiv.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\ofb\ofb_start.c
# End Source File
# End Group
# Begin Group "xts"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\modes\xts\xts_decrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\xts\xts_done.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\xts\xts_encrypt.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\xts\xts_init.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\xts\xts_mult_x.c
# End Source File
# Begin Source File

SOURCE=.\src\modes\xts\xts_test.c
# End Source File
# End Group
# End Group
# Begin Group "pk"

# PROP Default_Filter ""
# Begin Group "asn1"

# PROP Default_Filter ""
# Begin Group "der"

# PROP Default_Filter ""
# Begin Group "bit"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\asn1\der\bit\der_decode_bit_string.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\bit\der_encode_bit_string.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\bit\der_length_bit_string.c
# End Source File
# End Group
# Begin Group "boolean"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\asn1\der\boolean\der_decode_boolean.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\boolean\der_encode_boolean.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\boolean\der_length_boolean.c
# End Source File
# End Group
# Begin Group "choice"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\asn1\der\choice\der_decode_choice.c
# End Source File
# End Group
# Begin Group "ia5"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\asn1\der\ia5\der_decode_ia5_string.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\ia5\der_encode_ia5_string.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\ia5\der_length_ia5_string.c
# End Source File
# End Group
# Begin Group "integer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\asn1\der\integer\der_decode_integer.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\integer\der_encode_integer.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\integer\der_length_integer.c
# End Source File
# End Group
# Begin Group "object_identifier"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\asn1\der\object_identifier\der_decode_object_identifier.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\object_identifier\der_encode_object_identifier.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\object_identifier\der_length_object_identifier.c
# End Source File
# End Group
# Begin Group "octet"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\asn1\der\octet\der_decode_octet_string.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\octet\der_encode_octet_string.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\octet\der_length_octet_string.c
# End Source File
# End Group
# Begin Group "printable_string"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\asn1\der\printable_string\der_decode_printable_string.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\printable_string\der_encode_printable_string.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\printable_string\der_length_printable_string.c
# End Source File
# End Group
# Begin Group "sequence"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\asn1\der\sequence\der_decode_sequence_ex.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\sequence\der_decode_sequence_flexi.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\sequence\der_decode_sequence_multi.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\sequence\der_encode_sequence_ex.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\sequence\der_encode_sequence_multi.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\sequence\der_length_sequence.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\sequence\der_sequence_free.c
# End Source File
# End Group
# Begin Group "set"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\asn1\der\set\der_encode_set.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\set\der_encode_setof.c
# End Source File
# End Group
# Begin Group "short_integer"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\asn1\der\short_integer\der_decode_short_integer.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\short_integer\der_encode_short_integer.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\short_integer\der_length_short_integer.c
# End Source File
# End Group
# Begin Group "utctime"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\asn1\der\utctime\der_decode_utctime.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\utctime\der_encode_utctime.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\utctime\der_length_utctime.c
# End Source File
# End Group
# Begin Group "utf8"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\asn1\der\utf8\der_decode_utf8_string.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\utf8\der_encode_utf8_string.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\asn1\der\utf8\der_length_utf8_string.c
# End Source File
# End Group
# End Group
# End Group
# Begin Group "dsa"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\dsa\dsa_decrypt_key.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\dsa\dsa_encrypt_key.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\dsa\dsa_export.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\dsa\dsa_free.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\dsa\dsa_import.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\dsa\dsa_make_key.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\dsa\dsa_shared_secret.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\dsa\dsa_sign_hash.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\dsa\dsa_verify_hash.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\dsa\dsa_verify_key.c
# End Source File
# End Group
# Begin Group "ecc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\ecc\ecc.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_ansi_x963_export.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_ansi_x963_import.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_decrypt_key.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_encrypt_key.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_export.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_free.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_get_size.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_import.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_make_key.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_shared_secret.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_sign_hash.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_sizes.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_test.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ecc_verify_hash.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ltc_ecc_is_valid_idx.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ltc_ecc_map.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ltc_ecc_mul2add.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ltc_ecc_mulmod.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ltc_ecc_mulmod_timing.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ltc_ecc_points.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ltc_ecc_projective_add_point.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\ecc\ltc_ecc_projective_dbl_point.c
# End Source File
# End Group
# Begin Group "katja"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\katja\katja_decrypt_key.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\katja\katja_encrypt_key.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\katja\katja_export.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\katja\katja_exptmod.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\katja\katja_free.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\katja\katja_import.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\katja\katja_make_key.c
# End Source File
# End Group
# Begin Group "pkcs1"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\pkcs1\pkcs_1_i2osp.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\pkcs1\pkcs_1_mgf1.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\pkcs1\pkcs_1_oaep_decode.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\pkcs1\pkcs_1_oaep_encode.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\pkcs1\pkcs_1_os2ip.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\pkcs1\pkcs_1_pss_decode.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\pkcs1\pkcs_1_pss_encode.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\pkcs1\pkcs_1_v1_5_decode.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\pkcs1\pkcs_1_v1_5_encode.c
# End Source File
# End Group
# Begin Group "rsa"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pk\rsa\rsa_decrypt_key.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\rsa\rsa_encrypt_key.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\rsa\rsa_export.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\rsa\rsa_exptmod.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\rsa\rsa_free.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\rsa\rsa_import.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\rsa\rsa_make_key.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\rsa\rsa_sign_hash.c
# End Source File
# Begin Source File

SOURCE=.\src\pk\rsa\rsa_verify_hash.c
# End Source File
# End Group
# End Group
# Begin Group "prngs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\prngs\fortuna.c
# End Source File
# Begin Source File

SOURCE=.\src\prngs\rc4.c
# End Source File
# Begin Source File

SOURCE=.\src\prngs\rng_get_bytes.c
# End Source File
# Begin Source File

SOURCE=.\src\prngs\rng_make_prng.c
# End Source File
# Begin Source File

SOURCE=.\src\prngs\sober128.c
# End Source File
# Begin Source File

SOURCE=.\src\prngs\sober128tab.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\prngs\sprng.c
# End Source File
# Begin Source File

SOURCE=.\src\prngs\yarrow.c
# End Source File
# End Group
# Begin Group "headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\headers\tomcrypt.h
# End Source File
# Begin Source File

SOURCE=.\src\headers\tomcrypt_argchk.h
# End Source File
# Begin Source File

SOURCE=.\src\headers\tomcrypt_cfg.h
# End Source File
# Begin Source File

SOURCE=.\src\headers\tomcrypt_cipher.h
# End Source File
# Begin Source File

SOURCE=.\src\headers\tomcrypt_custom.h
# End Source File
# Begin Source File

SOURCE=.\src\headers\tomcrypt_hash.h
# End Source File
# Begin Source File

SOURCE=.\src\headers\tomcrypt_mac.h
# End Source File
# Begin Source File

SOURCE=.\src\headers\tomcrypt_macros.h
# End Source File
# Begin Source File

SOURCE=.\src\headers\tomcrypt_math.h
# End Source File
# Begin Source File

SOURCE=.\src\headers\tomcrypt_misc.h
# End Source File
# Begin Source File

SOURCE=.\src\headers\tomcrypt_pk.h
# End Source File
# Begin Source File

SOURCE=.\src\headers\tomcrypt_pkcs.h
# End Source File
# Begin Source File

SOURCE=.\src\headers\tomcrypt_prng.h
# End Source File
# End Group
# End Target
# End Project
