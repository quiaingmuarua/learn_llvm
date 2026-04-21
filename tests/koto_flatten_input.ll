define i32 @foo(i1 %cond) {
entry:
  br i1 %cond, label %then, label %else

then:
  ret i32 1

else:
  ret i32 2
}
