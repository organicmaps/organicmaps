CFLAGS += -I../src/headers -I./ -Wall -W

# ranlib tools
ifndef RANLIB
   RANLIB=ranlib
endif

OBJECTS = base64_test.o cipher_hash_test.o der_tests.o                                   \
dsa_test.o ecc_test.o mac_test.o modes_test.o pkcs_1_test.o rsa_test.o                   \
store_test.o test_driver.o x86_prof.o katja_test.o

ifndef LIBTEST_S
   LIBTEST_S=libtomcrypt_prof.a
endif

default: $(LIBTEST_S)

$(LIBTEST_S): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)
	$(RANLIB) $@

clean:
	rm -f *.o *.a

