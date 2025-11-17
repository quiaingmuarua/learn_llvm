; 简单测试模块
define i32 @add(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b
  ret i32 %sum
}

define void @foo() {
entry:
  ret void
}

declare void @external_fn(i32)
