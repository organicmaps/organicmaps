# How to run tests #
Tests must be run from android root android directory:
```bash
cd somewhere_in_your_system/omim/android
```
Then just run run_spoon_tests.sh script:
```bash
./tests/run_spoon_tests.sh
```
Static html report will be placed in **spoon-output** folder.

*NOTE* project must be compiled with debug configuration before running test:`cd tests; ant debug`
