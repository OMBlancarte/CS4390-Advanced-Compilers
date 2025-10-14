; ModuleID = '../inputs/matmul.c'
source_filename = "../inputs/matmul.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local void @matmul(ptr noundef %0, ptr noundef %1, ptr noundef %2) #0 {
  %4 = alloca ptr, align 8
  %5 = alloca ptr, align 8
  %6 = alloca ptr, align 8
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca double, align 8
  %10 = alloca i32, align 4
  store ptr %0, ptr %4, align 8
  store ptr %1, ptr %5, align 8
  store ptr %2, ptr %6, align 8
  store i32 0, ptr %7, align 4
  br label %11

11:                                               ; preds = %57, %3
  %12 = load i32, ptr %7, align 4
  %13 = icmp slt i32 %12, 512
  br i1 %13, label %14, label %60

14:                                               ; preds = %11
  store i32 0, ptr %8, align 4
  br label %15

15:                                               ; preds = %53, %14
  %16 = load i32, ptr %8, align 4
  %17 = icmp slt i32 %16, 512
  br i1 %17, label %18, label %56

18:                                               ; preds = %15
  store double 0.000000e+00, ptr %9, align 8
  store i32 0, ptr %10, align 4
  br label %19

19:                                               ; preds = %41, %18
  %20 = load i32, ptr %10, align 4
  %21 = icmp slt i32 %20, 512
  br i1 %21, label %22, label %44

22:                                               ; preds = %19
  %23 = load ptr, ptr %4, align 8
  %24 = load i32, ptr %7, align 4
  %25 = sext i32 %24 to i64
  %26 = getelementptr inbounds [512 x double], ptr %23, i64 %25
  %27 = load i32, ptr %10, align 4
  %28 = sext i32 %27 to i64
  %29 = getelementptr inbounds [512 x double], ptr %26, i64 0, i64 %28
  %30 = load double, ptr %29, align 8
  %31 = load ptr, ptr %5, align 8
  %32 = load i32, ptr %10, align 4
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds [512 x double], ptr %31, i64 %33
  %35 = load i32, ptr %8, align 4
  %36 = sext i32 %35 to i64
  %37 = getelementptr inbounds [512 x double], ptr %34, i64 0, i64 %36
  %38 = load double, ptr %37, align 8
  %39 = load double, ptr %9, align 8
  %40 = call double @llvm.fmuladd.f64(double %30, double %38, double %39)
  store double %40, ptr %9, align 8
  br label %41

41:                                               ; preds = %22
  %42 = load i32, ptr %10, align 4
  %43 = add nsw i32 %42, 1
  store i32 %43, ptr %10, align 4
  br label %19, !llvm.loop !6

44:                                               ; preds = %19
  %45 = load double, ptr %9, align 8
  %46 = load ptr, ptr %6, align 8
  %47 = load i32, ptr %7, align 4
  %48 = sext i32 %47 to i64
  %49 = getelementptr inbounds [512 x double], ptr %46, i64 %48
  %50 = load i32, ptr %8, align 4
  %51 = sext i32 %50 to i64
  %52 = getelementptr inbounds [512 x double], ptr %49, i64 0, i64 %51
  store double %45, ptr %52, align 8
  br label %53

53:                                               ; preds = %44
  %54 = load i32, ptr %8, align 4
  %55 = add nsw i32 %54, 1
  store i32 %55, ptr %8, align 4
  br label %15, !llvm.loop !8

56:                                               ; preds = %15
  br label %57

57:                                               ; preds = %56
  %58 = load i32, ptr %7, align 4
  %59 = add nsw i32 %58, 1
  store i32 %59, ptr %7, align 4
  br label %11, !llvm.loop !9

60:                                               ; preds = %11
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare double @llvm.fmuladd.f64(double, double, double) #1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca [512 x [512 x double]], align 16
  %3 = alloca [512 x [512 x double]], align 16
  %4 = alloca [512 x [512 x double]], align 16
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  store i32 0, ptr %5, align 4
  br label %7

7:                                                ; preds = %39, %0
  %8 = load i32, ptr %5, align 4
  %9 = icmp slt i32 %8, 512
  br i1 %9, label %10, label %42

10:                                               ; preds = %7
  store i32 0, ptr %6, align 4
  br label %11

11:                                               ; preds = %35, %10
  %12 = load i32, ptr %6, align 4
  %13 = icmp slt i32 %12, 512
  br i1 %13, label %14, label %38

14:                                               ; preds = %11
  %15 = load i32, ptr %5, align 4
  %16 = load i32, ptr %6, align 4
  %17 = add nsw i32 %15, %16
  %18 = sitofp i32 %17 to double
  %19 = load i32, ptr %5, align 4
  %20 = sext i32 %19 to i64
  %21 = getelementptr inbounds [512 x [512 x double]], ptr %2, i64 0, i64 %20
  %22 = load i32, ptr %6, align 4
  %23 = sext i32 %22 to i64
  %24 = getelementptr inbounds [512 x double], ptr %21, i64 0, i64 %23
  store double %18, ptr %24, align 8
  %25 = load i32, ptr %5, align 4
  %26 = load i32, ptr %6, align 4
  %27 = sub nsw i32 %25, %26
  %28 = sitofp i32 %27 to double
  %29 = load i32, ptr %5, align 4
  %30 = sext i32 %29 to i64
  %31 = getelementptr inbounds [512 x [512 x double]], ptr %3, i64 0, i64 %30
  %32 = load i32, ptr %6, align 4
  %33 = sext i32 %32 to i64
  %34 = getelementptr inbounds [512 x double], ptr %31, i64 0, i64 %33
  store double %28, ptr %34, align 8
  br label %35

35:                                               ; preds = %14
  %36 = load i32, ptr %6, align 4
  %37 = add nsw i32 %36, 1
  store i32 %37, ptr %6, align 4
  br label %11, !llvm.loop !10

38:                                               ; preds = %11
  br label %39

39:                                               ; preds = %38
  %40 = load i32, ptr %5, align 4
  %41 = add nsw i32 %40, 1
  store i32 %41, ptr %5, align 4
  br label %7, !llvm.loop !11

42:                                               ; preds = %7
  %43 = getelementptr inbounds [512 x [512 x double]], ptr %2, i64 0, i64 0
  %44 = getelementptr inbounds [512 x [512 x double]], ptr %3, i64 0, i64 0
  %45 = getelementptr inbounds [512 x [512 x double]], ptr %4, i64 0, i64 0
  call void @matmul(ptr noundef %43, ptr noundef %44, ptr noundef %45)
  %46 = getelementptr inbounds [512 x [512 x double]], ptr %4, i64 0, i64 511
  %47 = getelementptr inbounds [512 x double], ptr %46, i64 0, i64 511
  %48 = load double, ptr %47, align 8
  %49 = call i32 (ptr, ...) @printf(ptr noundef @.str, double noundef %48)
  ret i32 0
}

declare i32 @printf(ptr noundef, ...) #2

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #2 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 18.1.3 (1ubuntu1)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
!10 = distinct !{!10, !7}
!11 = distinct !{!11, !7}
