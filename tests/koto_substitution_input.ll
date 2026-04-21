define i32 @foo(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b
  br label %exit

exit:
  ret i32 %sum
}
