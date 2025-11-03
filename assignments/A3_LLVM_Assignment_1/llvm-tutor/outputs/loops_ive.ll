; ModuleID = '../outputs/loops_canonical.ll'
source_filename = "loops.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [10 x i8] c"Sum = %d\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 {
  br label %1

1:                                                ; preds = %9, %0
  %.016 = phi i32 [ 0, %0 ], [ %.1.lcssa, %9 ]
  %.025 = phi i32 [ 0, %0 ], [ %10, %9 ]
  br label %2

2:                                                ; preds = %5, %1
  %.04 = phi i32 [ 0, %1 ], [ %6, %5 ]
  %.13 = phi i32 [ %.016, %1 ], [ %4, %5 ]
  %3 = add nsw i32 %.025, %.04
  %4 = add nsw i32 %.13, %3
  br label %5

5:                                                ; preds = %2
  %6 = add nsw i32 %.04, 1
  %7 = icmp slt i32 %6, 2
  br i1 %7, label %2, label %8, !llvm.loop !6

8:                                                ; preds = %5
  %.1.lcssa = phi i32 [ %4, %5 ]
  br label %9

9:                                                ; preds = %8
  %10 = add nsw i32 %.025, 1
  %11 = icmp slt i32 %10, 3
  br i1 %11, label %1, label %12, !llvm.loop !8

12:                                               ; preds = %9
  %.01.lcssa = phi i32 [ %.1.lcssa, %9 ]
  %13 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %.01.lcssa)
  ret i32 0
}

declare i32 @printf(ptr noundef, ...) #1

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 21.1.3 (++20251007014011+450f52eec88f-1~exp1~20251007134114.40)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
