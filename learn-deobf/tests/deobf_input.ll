; RUN: This file is consumed by pass_deobf via CTest

define i32 @foo(i32 %a, i32 %b) {
entry:
  %obf = call i32 @__learn_llvm_obf_add_i32(i32 %a, i32 %b)
  ret i32 %obf
}

define internal i32 @__learn_llvm_obf_add_i32(i32 %x, i32 %y) {
entry:
  %sum = add i32 %x, %y
  ret i32 %sum
}

