export LLVM_DIR=/root/open_source/llvm-build/lib/cmake/llvm
cd /root/open_source/learn_llvm

cmake --build build


/root/open_source/llvm-build/bin/opt \
  -load-pass-plugin ./build/lib/learn_llvm_pass.so \
  -passes=hello-pass,junk-pass \
  -S ./learn-pass/tests/hello_input.ll \
  -o ./tmp/out_both.ll



/root/open_source/llvm-build/bin/clang \
  -O1 \
  -fpass-plugin=./build/lib/learn_llvm_pass.so \
  -S -emit-llvm \
  ./example/test.c \
  -o ./tmp/add_pass.ll

#generate aarch64 version
/root/open_source/llvm-build/bin/clang \
  --target=aarch64-linux-gnu \
  -O1 \
  -fpass-plugin=./build/lib/learn_llvm_pass.so \
  -fPIC -shared \
  ./example/test.c \
  -o ./tmp/libexample_arm64.so

#generate aarch64 version
/root/open_source/llvm-build/bin/clang \
  --target=aarch64-linux-gnu \
  -O1 \
  -fpass-plugin=./build/lib/learn_llvm_pass.so \
  ./example/test.c \
  -o ./tmp/example_arm64


#generate x86_64 version
/root/open_source/llvm-build/bin/clang \
  --target=x86_64-linux-gnu \
  -O1 \
  -fpass-plugin=./build/lib/learn_llvm_pass.so \
  ./example/test.c \
  -o ./tmp/example_x86_64


############################################

/root/open_source/llvm-build/bin/clang \
  -O1 \
  -fpass-plugin=./build/lib/Kotoamatsukami.so \
  -S -emit-llvm \
  ./example/test.c \
  -o ./tmp/Kotoamatsukami_after.ll



/root/open_source/llvm-build/bin/clang \
  --target=aarch64-linux-gnu \
  -O1 \
  -fpass-plugin=./build/lib/Kotoamatsukami.so \
  ./example/test.c \
  -o ./tmp/Kotoamatsukami_arm64





/root/open_source/llvm-build/bin/clang \
  --target=aarch64-linux-gnu \
  -O1 \
  -fpass-plugin=./build/lib/Kotoamatsukami.so \
  -fPIC -shared \
  ./example/test.c \
  -o ./tmp/Kotoamatsukami_arm64.so
