; ModuleID = '../outputs/iveTest_canonical.ll'
source_filename = "../inputs/iveTest.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local void @f(i32 noundef %0) #0 {
  %2 = call i32 (ptr, ...) @printf(ptr noundef @.str, i32 noundef %0)
  ret void
}

declare i32 @printf(ptr noundef, ...) #1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 {
  %1 = alloca [100 x i32], align 16
  br label %2

2:                                                ; preds = %6, %0
  %.012 = phi i32 [ 0, %0 ], [ %7, %6 ]
  %3 = mul nsw i32 %.012, 2
  %4 = sext i32 %.012 to i64
  %5 = getelementptr inbounds [100 x i32], ptr %1, i64 0, i64 %4
  store i32 %3, ptr %5, align 4
  br label %6

6:                                                ; preds = %2
  %7 = add nsw i32 %.012, 1
  %8 = icmp slt i32 %7, 100
  br i1 %8, label %2, label %9, !llvm.loop !6

9:                                                ; preds = %6
  br label %10

10:                                               ; preds = %14, %9
  %.03 = phi i32 [ 0, %9 ], [ %15, %14 ]
  %11 = sext i32 %.03 to i64
  %12 = getelementptr inbounds [100 x i32], ptr %1, i64 0, i64 %11
  %13 = load i32, ptr %12, align 4
  call void @f(i32 noundef %13)
  br label %14

14:                                               ; preds = %10
  %15 = add nsw i32 %.03, 1
  %16 = icmp slt i32 %15, 100
  br i1 %16, label %10, label %17, !llvm.loop !8

17:                                               ; preds = %14
  ret i32 0
}

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
