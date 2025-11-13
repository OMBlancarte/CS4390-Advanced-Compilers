; ModuleID = './test/branch_example.ll'
source_filename = "./test/branch_example.ll"

define void @branch_example(ptr %p, ptr %q) {
entry:
  store i32 1, ptr %p, align 4
  br i1 true, label %if, label %else

if:                                               ; preds = %entry
  store i32 2, ptr %p, align 4
  br label %exit

else:                                             ; preds = %entry
  store i32 3, ptr %p, align 4
  br label %exit

exit:                                             ; preds = %else, %if
  %v = load i32, ptr %p, align 4
  ret void
}
