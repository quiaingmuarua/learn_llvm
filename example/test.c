// 一个稍微复杂一点的 C 示例，用来更直观地观察 LLVM Pass 的效果。
// 编译命令（在项目根目录）：
//   /root/open_source/llvm-build/bin/clang \
//     -O1 \
//     -fpass-plugin=./lib/liblearn_llvm_pass.so \
//     -S -emit-llvm \
//     ./example/test.c \
//     -o ./tmp/add_pass.ll

#include <stdio.h>

// 一个简单的加法函数：你的 JunkPass 会在这里插入 junk1/junk2 等指令
int add(int a, int b) {
    // 保持实现简单，方便在 IR 中对比优化前后的样子
    return a + b;
}

// 再加一个有多步运算的函数，方便观察 Pass 在不同函数上的行为
int calc(int x, int y) {
    int t1 = x * 2;
    int t2 = y + 3;
    int t3 = t1 - t2;
    int r  = add(t3, x);
    return r;
}

int main(void) {
    int a = 10;
    int b = 20;

    int c = add(a, b);
    int d = calc(c, a);

    printf("a=%d, b=%d, c=%d, d=%d\n", a, b, c, d);
    return 0;
}
