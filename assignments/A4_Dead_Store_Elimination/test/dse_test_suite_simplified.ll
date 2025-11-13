; ModuleID = 'dse_test_suite.ll'
source_filename = "dse_test_suite.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.Point = type { i32, i32 }

; Function Attrs: noinline nounwind uwtable
define dso_local void @test1_simple_dead_store(ptr noundef %0) #0 {
  store i32 1, ptr %0, align 4
  store i32 2, ptr %0, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test2_multiple_dead_stores(ptr noundef %0) #0 {
  store i32 1, ptr %0, align 4
  store i32 2, ptr %0, align 4
  store i32 3, ptr %0, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test3_store_with_use(ptr noundef %0, ptr noundef %1) #0 {
  store i32 5, ptr %0, align 4
  %3 = load i32, ptr %0, align 4
  store i32 %3, ptr %1, align 4
  store i32 10, ptr %0, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test4_computed_dead_store(ptr noundef %0, i32 noundef %1) #0 {
  %3 = mul nsw i32 %1, 2
  store i32 %3, ptr %0, align 4
  %4 = add nsw i32 %1, 5
  store i32 %4, ptr %0, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test5_same_location_different_syntax(ptr noundef %0) #0 {
  %2 = getelementptr inbounds i32, ptr %0, i64 0
  store i32 100, ptr %2, align 4
  store i32 200, ptr %0, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test6_conditional_not_dead(ptr noundef %0, i32 noundef %1) #0 {
  store i32 1, ptr %0, align 4
  %3 = icmp ne i32 %1, 0
  br i1 %3, label %4, label %5

4:                                                ; preds = %2
  store i32 2, ptr %0, align 4
  br label %5

5:                                                ; preds = %4, %2
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test7_unconditional_overwrite(ptr noundef %0, i32 noundef %1) #0 {
  store i32 1, ptr %0, align 4
  %3 = icmp ne i32 %1, 0
  br i1 %3, label %4, label %5

4:                                                ; preds = %2
  store i32 2, ptr %0, align 4
  br label %6

5:                                                ; preds = %2
  store i32 3, ptr %0, align 4
  br label %6

6:                                                ; preds = %5, %4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test8_different_locations(ptr noundef %0, ptr noundef %1) #0 {
  store i32 10, ptr %0, align 4
  store i32 20, ptr %1, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test9_loop_dead_store(ptr noundef %0, i32 noundef %1) #0 {
  br label %3

3:                                                ; preds = %6, %2
  %.0 = phi i32 [ 0, %2 ], [ %7, %6 ]
  %4 = icmp slt i32 %.0, %1
  br i1 %4, label %5, label %8

5:                                                ; preds = %3
  store i32 %.0, ptr %0, align 4
  br label %6

6:                                                ; preds = %5
  %7 = add nsw i32 %.0, 1
  br label %3, !llvm.loop !6

8:                                                ; preds = %3
  store i32 999, ptr %0, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test10_dead_init_before_loop(ptr noundef %0, i32 noundef %1) #0 {
  store i32 0, ptr %0, align 4
  br label %3

3:                                                ; preds = %6, %2
  %.0 = phi i32 [ 0, %2 ], [ %7, %6 ]
  %4 = icmp slt i32 %.0, %1
  br i1 %4, label %5, label %8

5:                                                ; preds = %3
  store i32 %.0, ptr %0, align 4
  br label %6

6:                                                ; preds = %5
  %7 = add nsw i32 %.0, 1
  br label %3, !llvm.loop !8

8:                                                ; preds = %3
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test11_array_dead_stores(ptr noundef %0) #0 {
  %2 = getelementptr inbounds i32, ptr %0, i64 0
  store i32 1, ptr %2, align 4
  %3 = getelementptr inbounds i32, ptr %0, i64 0
  store i32 2, ptr %3, align 4
  %4 = getelementptr inbounds i32, ptr %0, i64 1
  store i32 10, ptr %4, align 4
  %5 = getelementptr inbounds i32, ptr %0, i64 2
  store i32 20, ptr %5, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @test12_early_return(ptr noundef %0, i32 noundef %1) #0 {
  store i32 1, ptr %0, align 4
  %3 = icmp ne i32 %1, 0
  br i1 %3, label %4, label %5

4:                                                ; preds = %2
  br label %6

5:                                                ; preds = %2
  store i32 2, ptr %0, align 4
  br label %6

6:                                                ; preds = %5, %4
  %.0 = phi i32 [ 0, %4 ], [ 1, %5 ]
  ret i32 %.0
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test13_chain_of_dead_stores(ptr noundef %0) #0 {
  store i32 1, ptr %0, align 4
  store i32 2, ptr %0, align 4
  store i32 3, ptr %0, align 4
  store i32 4, ptr %0, align 4
  store i32 5, ptr %0, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test14_pointer_arithmetic(ptr noundef %0) #0 {
  store i32 10, ptr %0, align 4
  %2 = getelementptr inbounds i32, ptr %0, i64 0
  store i32 20, ptr %2, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test15_struct_members(ptr noundef %0) #0 {
  %2 = getelementptr inbounds nuw %struct.Point, ptr %0, i32 0, i32 0
  store i32 1, ptr %2, align 4
  %3 = getelementptr inbounds nuw %struct.Point, ptr %0, i32 0, i32 0
  store i32 2, ptr %3, align 4
  %4 = getelementptr inbounds nuw %struct.Point, ptr %0, i32 0, i32 1
  store i32 10, ptr %4, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test16_call_may_use(ptr noundef %0) #0 {
  store i32 5, ptr %0, align 4
  call void @external_func(ptr noundef %0)
  store i32 10, ptr %0, align 4
  ret void
}

declare void @external_func(ptr noundef) #1

; Function Attrs: noinline nounwind uwtable
define dso_local void @test17_after_volatile(ptr noundef %0, ptr noundef %1) #0 {
  store i32 1, ptr %0, align 4
  %3 = load volatile i32, ptr %1, align 4
  store i32 2, ptr %0, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test18_complex_postdom(ptr noundef %0, i32 noundef %1, i32 noundef %2) #0 {
  store i32 0, ptr %0, align 4
  %4 = icmp sgt i32 %1, 0
  br i1 %4, label %5, label %10

5:                                                ; preds = %3
  %6 = icmp sgt i32 %2, 0
  br i1 %6, label %7, label %8

7:                                                ; preds = %5
  store i32 1, ptr %0, align 4
  br label %9

8:                                                ; preds = %5
  store i32 2, ptr %0, align 4
  br label %9

9:                                                ; preds = %8, %7
  br label %11

10:                                               ; preds = %3
  store i32 3, ptr %0, align 4
  br label %11

11:                                               ; preds = %10, %9
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test19_partial_loop(ptr noundef %0, i32 noundef %1) #0 {
  store i32 100, ptr %0, align 4
  br label %3

3:                                                ; preds = %6, %2
  %.0 = phi i32 [ 0, %2 ], [ %7, %6 ]
  %4 = icmp slt i32 %.0, %1
  br i1 %4, label %5, label %8

5:                                                ; preds = %3
  store i32 %.0, ptr %0, align 4
  br label %6

6:                                                ; preds = %5
  %7 = add nsw i32 %.0, 1
  br label %3, !llvm.loop !9

8:                                                ; preds = %3
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local void @test20_known_indices(ptr noundef %0) #0 {
  %2 = getelementptr inbounds i32, ptr %0, i64 5
  store i32 42, ptr %2, align 4
  %3 = getelementptr inbounds i32, ptr %0, i64 5
  store i32 99, ptr %3, align 4
  %4 = getelementptr inbounds i32, ptr %0, i64 3
  store i32 10, ptr %4, align 4
  ret void
}

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca [10 x i32], align 16
  %3 = alloca %struct.Point, align 4
  call void @test1_simple_dead_store(ptr noundef %1)
  call void @test2_multiple_dead_stores(ptr noundef %1)
  %4 = getelementptr inbounds [10 x i32], ptr %2, i64 0, i64 0
  call void @test3_store_with_use(ptr noundef %1, ptr noundef %4)
  call void @test4_computed_dead_store(ptr noundef %1, i32 noundef 5)
  call void @test5_same_location_different_syntax(ptr noundef %1)
  call void @test6_conditional_not_dead(ptr noundef %1, i32 noundef 1)
  call void @test7_unconditional_overwrite(ptr noundef %1, i32 noundef 1)
  %5 = getelementptr inbounds [10 x i32], ptr %2, i64 0, i64 1
  call void @test8_different_locations(ptr noundef %1, ptr noundef %5)
  call void @test9_loop_dead_store(ptr noundef %1, i32 noundef 10)
  call void @test10_dead_init_before_loop(ptr noundef %1, i32 noundef 10)
  %6 = getelementptr inbounds [10 x i32], ptr %2, i64 0, i64 0
  call void @test11_array_dead_stores(ptr noundef %6)
  %7 = call i32 @test12_early_return(ptr noundef %1, i32 noundef 0)
  call void @test13_chain_of_dead_stores(ptr noundef %1)
  call void @test14_pointer_arithmetic(ptr noundef %1)
  call void @test15_struct_members(ptr noundef %3)
  call void @test18_complex_postdom(ptr noundef %1, i32 noundef 1, i32 noundef 1)
  call void @test19_partial_loop(ptr noundef %1, i32 noundef 5)
  %8 = getelementptr inbounds [10 x i32], ptr %2, i64 0, i64 0
  call void @test20_known_indices(ptr noundef %8)
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
