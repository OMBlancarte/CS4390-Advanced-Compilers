; ModuleID = 'dse_test_suite.c'
source_filename = "dse_test_suite.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.Point = type { i32, i32 }

; Function Attrs: noinline nounwind uwtable
define dso_local void @test1_simple_dead_store(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  store i32 1, ptr %3, align 4
  %4 = load ptr, ptr %2, align 8
  store i32 2, ptr %4, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test2_multiple_dead_stores(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  store i32 1, ptr %3, align 4
  %4 = load ptr, ptr %2, align 8
  store i32 2, ptr %4, align 4
  %5 = load ptr, ptr %2, align 8
  store i32 3, ptr %5, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test3_store_with_use(ptr noundef %0, ptr noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca ptr, align 8
  store ptr %0, ptr %3, align 8
  store ptr %1, ptr %4, align 8
  %5 = load ptr, ptr %3, align 8
  store i32 5, ptr %5, align 4
  %6 = load ptr, ptr %3, align 8
  %7 = load i32, ptr %6, align 4
  %8 = load ptr, ptr %4, align 8
  store i32 %7, ptr %8, align 4
  %9 = load ptr, ptr %3, align 8
  store i32 10, ptr %9, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test4_computed_dead_store(ptr noundef %0, i32 noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca i32, align 4
  store ptr %0, ptr %3, align 8
  store i32 %1, ptr %4, align 4
  %5 = load i32, ptr %4, align 4
  %6 = mul nsw i32 %5, 2
  %7 = load ptr, ptr %3, align 8
  store i32 %6, ptr %7, align 4
  %8 = load i32, ptr %4, align 4
  %9 = add nsw i32 %8, 5
  %10 = load ptr, ptr %3, align 8
  store i32 %9, ptr %10, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test5_same_location_different_syntax(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  %4 = getelementptr inbounds i32, ptr %3, i64 0
  store i32 100, ptr %4, align 4
  %5 = load ptr, ptr %2, align 8
  store i32 200, ptr %5, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test6_conditional_not_dead(ptr noundef %0, i32 noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca i32, align 4
  store ptr %0, ptr %3, align 8
  store i32 %1, ptr %4, align 4
  %5 = load ptr, ptr %3, align 8
  store i32 1, ptr %5, align 4
  %6 = load i32, ptr %4, align 4
  %7 = icmp ne i32 %6, 0
  br i1 %7, label %8, label %10

8:                                                ; preds = %2
  %9 = load ptr, ptr %3, align 8
  store i32 2, ptr %9, align 4
  br label %10

10:                                               ; preds = %8, %2
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test7_unconditional_overwrite(ptr noundef %0, i32 noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca i32, align 4
  store ptr %0, ptr %3, align 8
  store i32 %1, ptr %4, align 4
  %5 = load ptr, ptr %3, align 8
  store i32 1, ptr %5, align 4
  %6 = load i32, ptr %4, align 4
  %7 = icmp ne i32 %6, 0
  br i1 %7, label %8, label %10

8:                                                ; preds = %2
  %9 = load ptr, ptr %3, align 8
  store i32 2, ptr %9, align 4
  br label %12

10:                                               ; preds = %2
  %11 = load ptr, ptr %3, align 8
  store i32 3, ptr %11, align 4
  br label %12

12:                                               ; preds = %10, %8
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test8_different_locations(ptr noundef %0, ptr noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca ptr, align 8
  store ptr %0, ptr %3, align 8
  store ptr %1, ptr %4, align 8
  %5 = load ptr, ptr %3, align 8
  store i32 10, ptr %5, align 4
  %6 = load ptr, ptr %4, align 8
  store i32 20, ptr %6, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test9_loop_dead_store(ptr noundef %0, i32 noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store ptr %0, ptr %3, align 8
  store i32 %1, ptr %4, align 4
  store i32 0, ptr %5, align 4
  br label %6

6:                                                ; preds = %13, %2
  %7 = load i32, ptr %5, align 4
  %8 = load i32, ptr %4, align 4
  %9 = icmp slt i32 %7, %8
  br i1 %9, label %10, label %16

10:                                               ; preds = %6
  %11 = load i32, ptr %5, align 4
  %12 = load ptr, ptr %3, align 8
  store i32 %11, ptr %12, align 4
  br label %13

13:                                               ; preds = %10
  %14 = load i32, ptr %5, align 4
  %15 = add nsw i32 %14, 1
  store i32 %15, ptr %5, align 4
  br label %6, !llvm.loop !6

16:                                               ; preds = %6
  %17 = load ptr, ptr %3, align 8
  store i32 999, ptr %17, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test10_dead_init_before_loop(ptr noundef %0, i32 noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store ptr %0, ptr %3, align 8
  store i32 %1, ptr %4, align 4
  %6 = load ptr, ptr %3, align 8
  store i32 0, ptr %6, align 4
  store i32 0, ptr %5, align 4
  br label %7

7:                                                ; preds = %14, %2
  %8 = load i32, ptr %5, align 4
  %9 = load i32, ptr %4, align 4
  %10 = icmp slt i32 %8, %9
  br i1 %10, label %11, label %17

11:                                               ; preds = %7
  %12 = load i32, ptr %5, align 4
  %13 = load ptr, ptr %3, align 8
  store i32 %12, ptr %13, align 4
  br label %14

14:                                               ; preds = %11
  %15 = load i32, ptr %5, align 4
  %16 = add nsw i32 %15, 1
  store i32 %16, ptr %5, align 4
  br label %7, !llvm.loop !8

17:                                               ; preds = %7
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test11_array_dead_stores(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  %4 = getelementptr inbounds i32, ptr %3, i64 0
  store i32 1, ptr %4, align 4
  %5 = load ptr, ptr %2, align 8
  %6 = getelementptr inbounds i32, ptr %5, i64 0
  store i32 2, ptr %6, align 4
  %7 = load ptr, ptr %2, align 8
  %8 = getelementptr inbounds i32, ptr %7, i64 1
  store i32 10, ptr %8, align 4
  %9 = load ptr, ptr %2, align 8
  %10 = getelementptr inbounds i32, ptr %9, i64 2
  store i32 20, ptr %10, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test12_early_return(ptr noundef %0, i32 noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca ptr, align 8
  %5 = alloca i32, align 4
  store ptr %0, ptr %4, align 8
  store i32 %1, ptr %5, align 4
  %6 = load ptr, ptr %4, align 8
  store i32 1, ptr %6, align 4
  %7 = load i32, ptr %5, align 4
  %8 = icmp ne i32 %7, 0
  br i1 %8, label %9, label %10

9:                                                ; preds = %2
  store i32 0, ptr %3, align 4
  br label %12

10:                                               ; preds = %2
  %11 = load ptr, ptr %4, align 8
  store i32 2, ptr %11, align 4
  store i32 1, ptr %3, align 4
  br label %12

12:                                               ; preds = %10, %9
  %13 = load i32, ptr %3, align 4
  ret i32 %13
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test13_chain_of_dead_stores(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  store i32 1, ptr %3, align 4
  %4 = load ptr, ptr %2, align 8
  store i32 2, ptr %4, align 4
  %5 = load ptr, ptr %2, align 8
  store i32 3, ptr %5, align 4
  %6 = load ptr, ptr %2, align 8
  store i32 4, ptr %6, align 4
  %7 = load ptr, ptr %2, align 8
  store i32 5, ptr %7, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test14_pointer_arithmetic(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  store i32 10, ptr %3, align 4
  %4 = load ptr, ptr %2, align 8
  %5 = getelementptr inbounds i32, ptr %4, i64 0
  store i32 20, ptr %5, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test15_struct_members(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  %4 = getelementptr inbounds nuw %struct.Point, ptr %3, i32 0, i32 0
  store i32 1, ptr %4, align 4
  %5 = load ptr, ptr %2, align 8
  %6 = getelementptr inbounds nuw %struct.Point, ptr %5, i32 0, i32 0
  store i32 2, ptr %6, align 4
  %7 = load ptr, ptr %2, align 8
  %8 = getelementptr inbounds nuw %struct.Point, ptr %7, i32 0, i32 1
  store i32 10, ptr %8, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test16_call_may_use(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  store i32 5, ptr %3, align 4
  %4 = load ptr, ptr %2, align 8
  call void @external_func(ptr noundef %4)
  %5 = load ptr, ptr %2, align 8
  store i32 10, ptr %5, align 4
  ret void
}

declare void @external_func(ptr noundef) #1

; Function Attrs: noinline nounwind uwtable
define dso_local void @test17_after_volatile(ptr noundef %0, ptr noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca ptr, align 8
  %5 = alloca i32, align 4
  store ptr %0, ptr %3, align 8
  store ptr %1, ptr %4, align 8
  %6 = load ptr, ptr %3, align 8
  store i32 1, ptr %6, align 4
  %7 = load ptr, ptr %4, align 8
  %8 = load volatile i32, ptr %7, align 4
  store i32 %8, ptr %5, align 4
  %9 = load ptr, ptr %3, align 8
  store i32 2, ptr %9, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test18_complex_postdom(ptr noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  %4 = alloca ptr, align 8
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  store ptr %0, ptr %4, align 8
  store i32 %1, ptr %5, align 4
  store i32 %2, ptr %6, align 4
  %7 = load ptr, ptr %4, align 8
  store i32 0, ptr %7, align 4
  %8 = load i32, ptr %5, align 4
  %9 = icmp sgt i32 %8, 0
  br i1 %9, label %10, label %18

10:                                               ; preds = %3
  %11 = load i32, ptr %6, align 4
  %12 = icmp sgt i32 %11, 0
  br i1 %12, label %13, label %15

13:                                               ; preds = %10
  %14 = load ptr, ptr %4, align 8
  store i32 1, ptr %14, align 4
  br label %17

15:                                               ; preds = %10
  %16 = load ptr, ptr %4, align 8
  store i32 2, ptr %16, align 4
  br label %17

17:                                               ; preds = %15, %13
  br label %20

18:                                               ; preds = %3
  %19 = load ptr, ptr %4, align 8
  store i32 3, ptr %19, align 4
  br label %20

20:                                               ; preds = %18, %17
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test19_partial_loop(ptr noundef %0, i32 noundef %1) #0 {
  %3 = alloca ptr, align 8
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store ptr %0, ptr %3, align 8
  store i32 %1, ptr %4, align 4
  %6 = load ptr, ptr %3, align 8
  store i32 100, ptr %6, align 4
  store i32 0, ptr %5, align 4
  br label %7

7:                                                ; preds = %14, %2
  %8 = load i32, ptr %5, align 4
  %9 = load i32, ptr %4, align 4
  %10 = icmp slt i32 %8, %9
  br i1 %10, label %11, label %17

11:                                               ; preds = %7
  %12 = load i32, ptr %5, align 4
  %13 = load ptr, ptr %3, align 8
  store i32 %12, ptr %13, align 4
  br label %14

14:                                               ; preds = %11
  %15 = load i32, ptr %5, align 4
  %16 = add nsw i32 %15, 1
  store i32 %16, ptr %5, align 4
  br label %7, !llvm.loop !9

17:                                               ; preds = %7
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test20_known_indices(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  %4 = getelementptr inbounds i32, ptr %3, i64 5
  store i32 42, ptr %4, align 4
  %5 = load ptr, ptr %2, align 8
  %6 = getelementptr inbounds i32, ptr %5, i64 5
  store i32 99, ptr %6, align 4
  %7 = load ptr, ptr %2, align 8
  %8 = getelementptr inbounds i32, ptr %7, i64 3
  store i32 10, ptr %8, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca [10 x i32], align 16
  %4 = alloca %struct.Point, align 4
  store i32 0, ptr %1, align 4
  call void @test1_simple_dead_store(ptr noundef %2)
  call void @test2_multiple_dead_stores(ptr noundef %2)
  %5 = getelementptr inbounds [10 x i32], ptr %3, i64 0, i64 0
  call void @test3_store_with_use(ptr noundef %2, ptr noundef %5)
  call void @test4_computed_dead_store(ptr noundef %2, i32 noundef 5)
  call void @test5_same_location_different_syntax(ptr noundef %2)
  call void @test6_conditional_not_dead(ptr noundef %2, i32 noundef 1)
  call void @test7_unconditional_overwrite(ptr noundef %2, i32 noundef 1)
  %6 = getelementptr inbounds [10 x i32], ptr %3, i64 0, i64 1
  call void @test8_different_locations(ptr noundef %2, ptr noundef %6)
  call void @test9_loop_dead_store(ptr noundef %2, i32 noundef 10)
  call void @test10_dead_init_before_loop(ptr noundef %2, i32 noundef 10)
  %7 = getelementptr inbounds [10 x i32], ptr %3, i64 0, i64 0
  call void @test11_array_dead_stores(ptr noundef %7)
  %8 = call i32 @test12_early_return(ptr noundef %2, i32 noundef 0)
  call void @test13_chain_of_dead_stores(ptr noundef %2)
  call void @test14_pointer_arithmetic(ptr noundef %2)
  call void @test15_struct_members(ptr noundef %4)
  call void @test18_complex_postdom(ptr noundef %2, i32 noundef 1, i32 noundef 1)
  call void @test19_partial_loop(ptr noundef %2, i32 noundef 5)
  %9 = getelementptr inbounds [10 x i32], ptr %3, i64 0, i64 0
  call void @test20_known_indices(ptr noundef %9)
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
!5 = !{!"Ubuntu clang version 21.1.5 (++20251023083335+45afac62e373-1~exp1~20251023083454.54)"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
