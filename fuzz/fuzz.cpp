#include<cstdio>
#include<zipfile.h>

/* Fuzzer application for zip parsing. */

int main(int argc, char **argv) {
  try {
    ZipFile zf(argv[1]);
  } catch(const std::exception &e) {
  }
  // Let unexpected errors bubble out.
  // "They should never happen."
  return 0;
}
