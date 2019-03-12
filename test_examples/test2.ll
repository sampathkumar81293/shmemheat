; ModuleID = 'test2.c'
source_filename = "test2.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@src = internal global [12 x i32] zeroinitializer, align 16
@.str = private unnamed_addr constant [14 x i8] c"My pe is :%d\0A\00", align 1
@.str.1 = private unnamed_addr constant [4 x i8] c"%d \00", align 1
@.str.2 = private unnamed_addr constant [2 x i8] c"\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32*, align 8
  %6 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  store i32 6, i32* %6, align 4
  store i32 0, i32* %4, align 4
  br label %7

; <label>:7:                                      ; preds = %15, %0
  %8 = load i32, i32* %4, align 4
  %9 = icmp slt i32 %8, 12
  br i1 %9, label %10, label %18

; <label>:10:                                     ; preds = %7
  %11 = load i32, i32* %4, align 4
  %12 = load i32, i32* %4, align 4
  %13 = sext i32 %12 to i64
  %14 = getelementptr inbounds [12 x i32], [12 x i32]* @src, i64 0, i64 %13
  store i32 %11, i32* %14, align 4
  br label %15

; <label>:15:                                     ; preds = %10
  %16 = load i32, i32* %4, align 4
  %17 = add nsw i32 %16, 1
  store i32 %17, i32* %4, align 4
  br label %7

; <label>:18:                                     ; preds = %7
  %19 = call i32 (...) @shmem_init()
  %20 = call i32 (...) @shmem_n_pes()
  store i32 %20, i32* %3, align 4
  %21 = call i32 (...) @shmem_my_pe()
  store i32 %21, i32* %2, align 4
  %22 = call i32 (i64, ...) bitcast (i32 (...)* @shmem_malloc to i32 (i64, ...)*)(i64 4)
  %23 = sext i32 %22 to i64
  %24 = inttoptr i64 %23 to i32*
  store i32* %24, i32** %5, align 8
  %25 = load i32, i32* %2, align 4
  %26 = load i32, i32* %6, align 4
  %27 = mul nsw i32 %25, %26
  %28 = load i32, i32* %3, align 4
  %29 = sdiv i32 %27, %28
  store i32 %29, i32* %4, align 4
  br label %30

; <label>:30:                                     ; preds = %61, %18
  %31 = load i32, i32* %4, align 4
  %32 = load i32, i32* %2, align 4
  %33 = add nsw i32 %32, 1
  %34 = load i32, i32* %6, align 4
  %35 = mul nsw i32 %33, %34
  %36 = load i32, i32* %3, align 4
  %37 = sdiv i32 %35, %36
  %38 = icmp slt i32 %31, %37
  br i1 %38, label %39, label %64

; <label>:39:                                     ; preds = %30
  %40 = load i32*, i32** %5, align 8
  %41 = load i32, i32* %4, align 4
  %42 = sext i32 %41 to i64
  %43 = getelementptr inbounds [12 x i32], [12 x i32]* @src, i64 0, i64 %42
  %44 = call i32 (i32*, i32*, i32, i32, ...) bitcast (i32 (...)* @shmem_int_get to i32 (i32*, i32*, i32, i32, ...)*)(i32* %40, i32* %43, i32 1, i32 1)
  %45 = load i32, i32* %4, align 4
  %46 = sext i32 %45 to i64
  %47 = getelementptr inbounds [12 x i32], [12 x i32]* @src, i64 0, i64 %46
  %48 = load i32, i32* %4, align 4
  %49 = load i32, i32* %6, align 4
  %50 = add nsw i32 %48, %49
  %51 = sext i32 %50 to i64
  %52 = getelementptr inbounds [12 x i32], [12 x i32]* @src, i64 0, i64 %51
  %53 = call i32 (i32*, i32*, i32, i32, ...) bitcast (i32 (...)* @shmem_int_put to i32 (i32*, i32*, i32, i32, ...)*)(i32* %47, i32* %52, i32 1, i32 1)
  %54 = load i32, i32* %4, align 4
  %55 = load i32, i32* %6, align 4
  %56 = add nsw i32 %54, %55
  %57 = sext i32 %56 to i64
  %58 = getelementptr inbounds [12 x i32], [12 x i32]* @src, i64 0, i64 %57
  %59 = load i32*, i32** %5, align 8
  %60 = call i32 (i32*, i32*, i32, i32, ...) bitcast (i32 (...)* @shmem_int_put to i32 (i32*, i32*, i32, i32, ...)*)(i32* %58, i32* %59, i32 1, i32 1)
  br label %61

; <label>:61:                                     ; preds = %39
  %62 = load i32, i32* %4, align 4
  %63 = add nsw i32 %62, 1
  store i32 %63, i32* %4, align 4
  br label %30

; <label>:64:                                     ; preds = %30
  %65 = call i32 (...) @shmem_barrier_all()
  %66 = load i32, i32* %2, align 4
  %67 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([14 x i8], [14 x i8]* @.str, i32 0, i32 0), i32 %66)
  store i32 0, i32* %4, align 4
  br label %68

; <label>:68:                                     ; preds = %77, %64
  %69 = load i32, i32* %4, align 4
  %70 = icmp slt i32 %69, 12
  br i1 %70, label %71, label %80

; <label>:71:                                     ; preds = %68
  %72 = load i32, i32* %4, align 4
  %73 = sext i32 %72 to i64
  %74 = getelementptr inbounds [12 x i32], [12 x i32]* @src, i64 0, i64 %73
  %75 = load i32, i32* %74, align 4
  %76 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.1, i32 0, i32 0), i32 %75)
  br label %77

; <label>:77:                                     ; preds = %71
  %78 = load i32, i32* %4, align 4
  %79 = add nsw i32 %78, 1
  store i32 %79, i32* %4, align 4
  br label %68

; <label>:80:                                     ; preds = %68
  %81 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str.2, i32 0, i32 0))
  %82 = call i32 (...) @shmem_finalize()
  ret i32 0
}

declare i32 @shmem_init(...) #1

declare i32 @shmem_n_pes(...) #1

declare i32 @shmem_my_pe(...) #1

declare i32 @shmem_malloc(...) #1

declare i32 @shmem_int_get(...) #1

declare i32 @shmem_int_put(...) #1

declare i32 @shmem_barrier_all(...) #1

declare i32 @printf(i8*, ...) #1

declare i32 @shmem_finalize(...) #1

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 4.0.1 (https://github.com/llvm-mirror/clang.git 3c8961bedc65c9a15cbe67a2ef385a0938f7cfef) (https://github.com/llvm-mirror/llvm.git c8fccc53ed66d505898f8850bcc690c977a7c9a7)"}
