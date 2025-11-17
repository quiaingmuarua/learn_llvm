export LLVM_DIR=/root/open_source/llvm-build/lib/cmake/llvm


cmake .
cmake --build .



/root/open_source/llvm-build/bin/opt \
  -load-pass-plugin ./lib/liblearn_llvm_pass.so \
  -passes=hello-pass,junk-pass \
  -S ./learn-pass/tests/hello_input.ll \
  -o ./tmp/out_both.ll


cd /root/open_source/learn_llvm

/root/open_source/llvm-build/bin/clang \
  -O0 \
  -fpass-plugin=./lib/liblearn_llvm_pass.so \
  -S -emit-llvm \
  ./example/test.c \
  -o ./tmp/add_pass.ll
