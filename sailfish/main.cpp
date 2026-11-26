// Everything lives in liborganicmaps.so. On aarch64 libhybris GL drivers keep their current
// context in bionic TLS slots right after the thread pointer, where the static linker would place
// an executable's own thread_local data; shared library TLS is laid out by the patched loader.
int OrganicMapsMain(int argc, char * argv[]);

// The Harbour validator requires main() in the dynamic symbol table of apps using Sailfish Silica.
__attribute__((visibility("default"))) int main(int argc, char * argv[])
{
  return OrganicMapsMain(argc, argv);
}
