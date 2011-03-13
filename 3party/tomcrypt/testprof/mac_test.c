/* test pmac/omac/hmac */
#include <tomcrypt_test.h>

int mac_test(void)
{
#ifdef LTC_HMAC
   DO(hmac_test()); 
#endif
#ifdef LTC_PMAC
   DO(pmac_test()); 
#endif
#ifdef LTC_OMAC
   DO(omac_test()); 
#endif
#ifdef LTC_XCBC
   DO(xcbc_test());
#endif
#ifdef LTC_F9_MODE
   DO(f9_test());
#endif
#ifdef LTC_EAX_MODE
   DO(eax_test());  
#endif
#ifdef LTC_OCB_MODE
   DO(ocb_test());  
#endif
#ifdef LTC_CCM_MODE
   DO(ccm_test());
#endif
#ifdef LTC_GCM_MODE
   DO(gcm_test());
#endif
#ifdef LTC_PELICAN
   DO(pelican_test());
#endif
   return 0;
}

/* $Source: /cvs/libtom/libtomcrypt/testprof/mac_test.c,v $ */
/* $Revision: 1.6 $ */
/* $Date: 2007/05/12 14:32:35 $ */
