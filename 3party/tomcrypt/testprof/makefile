CFLAGS += -I../src/headers -I./ -Wall -W

# ranlib tools
ifndef RANLIB
   RANLIB=ranlib
endif

OBJECTS = base64_test.o cipher_hash_test.o der_tests.o no_prng.o                         \
dsa_test.o ecc_test.o mac_test.o misc_test.o modes_test.o pkcs_1_test.o rsa_test.o       \
store_test.o test_driver.o x86_prof.o katja_test.o dh_test.o                             \
pkcs_1_pss_test.o pkcs_1_oaep_test.o pkcs_1_emsa_test.o pkcs_1_eme_test.o

ifndef LIBTEST_S
   LIBTEST_S=libtomcrypt_prof.a
endif

default: $(LIBTEST_S)

$(LIBTEST_S): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)
	$(RANLIB) $@

clean:
	rm -f *.o *.a *.gcov *.gcda *.gcno

