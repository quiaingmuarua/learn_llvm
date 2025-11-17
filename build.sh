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
  -O1 \
  -fpass-plugin=./lib/liblearn_llvm_pass.so \
  -S -emit-llvm \
  ./example/test.c \
  -o ./tmp/add_pass.ll


/root/open_source/llvm-build/bin/clang \
  --target=aarch64-linux-gnu \
  -O1 \
  -fpass-plugin=./lib/liblearn_llvm_pass.so \
  -fPIC -shared \
  ./example/test.c \
  -o ./tmp/libexample_arm64.so


/root/open_source/llvm-build/bin/clang \
  --target=aarch64-linux-gnu \
  -O1 \
  -fpass-plugin=./lib/liblearn_llvm_pass.so \
  ./example/test.c \
  -o ./tmp/example_arm64