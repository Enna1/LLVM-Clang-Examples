# Taint Propagation via Dataflow

### Description

LLVM/Clang Version: test on llvm7.0.0, llvm8.0.0

This llvm transform pass implements a flow-sensitive, field- and context-insensitive taint propagation based on classic dataflow analysis. 

It attaches !taint metadata to instructions. For a given instruction, the metadata, if present, indicates the set of tainted operands before this instruction executed.

### Build

```shell
$ cd /building-a-JIT-for-BF/bf-jit
$ mkdir build && cd build
$ cmake ..
$ make
```

### Example

Take `testcase/test_global.c` as a simple example.

```c
#include <stdio.h>
#include <stdlib.h>

int x;

int main(int argc, char **argv)
{
	FILE* inf = fopen(argv[1], "r");
	fseek(inf, 0, SEEK_END);
	long size = ftell(inf);
	rewind(inf);
	char* buffer = malloc(size+1);
	fread(buffer, size, 1, inf);
	buffer[size] = '\0';
	fclose(inf);
	x = buffer[1];
	free(buffer);
	return x;
}
```

Build LLVM IR from test_global.c

```shell
$ cd testcase
$ clang -S -emit-llvm test_global.c
```

Run this taint propagation pass, and get the output.

```shell
$ cd testcase
$ ../build/test-tp ./test_global.ll -o test_global.tp.ll
```

The test_global.tp.ll should contain code similar to:

```
; ModuleID = './test_global.ll'
source_filename = "test_global.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }

@.str = private unnamed_addr constant [2 x i8] c"r\00", align 1
@x = common dso_local global i32 0, align 4

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main(i32 %argc, i8** %argv) #0 {
entry:
  %arrayidx = getelementptr inbounds i8*, i8** %argv, i64 1
  %0 = load i8*, i8** %arrayidx, align 8
  %call = call %struct._IO_FILE* @fopen(i8* %0, i8* getelementptr inbounds ([2 x i8], [2 x i8]* @.str, i32 0, i32 0))
  %call1 = call i32 @fseek(%struct._IO_FILE* %call, i64 0, i32 2)
  %call2 = call i64 @ftell(%struct._IO_FILE* %call)
  call void @rewind(%struct._IO_FILE* %call)
  %add = add nsw i64 %call2, 1
  %call3 = call noalias i8* @malloc(i64 %add) #3
  %call4 = call i64 @fread(i8* %call3, i64 %call2, i64 1, %struct._IO_FILE* %call)
  %arrayidx5 = getelementptr inbounds i8, i8* %call3, i64 %call2, !taint !2
  store i8 0, i8* %arrayidx5, align 1, !taint !3
  %call6 = call i32 @fclose(%struct._IO_FILE* %call)
  %arrayidx7 = getelementptr inbounds i8, i8* %call3, i64 1, !taint !2
  %1 = load i8, i8* %arrayidx7, align 1, !taint !4
  %conv = sext i8 %1 to i32, !taint !5
  store i32 %conv, i32* @x, align 4, !taint !6
  call void @free(i8* %call3) #3, !taint !2
  %2 = load i32, i32* @x, align 4
  ret i32 %2, !taint !7
}

declare dso_local %struct._IO_FILE* @fopen(i8*, i8*) #1

declare dso_local i32 @fseek(%struct._IO_FILE*, i64, i32) #1

declare dso_local i64 @ftell(%struct._IO_FILE*) #1

declare dso_local void @rewind(%struct._IO_FILE*) #1

; Function Attrs: nounwind
declare dso_local noalias i8* @malloc(i64) #2

declare dso_local i64 @fread(i8*, i64, i64, %struct._IO_FILE*) #1

declare dso_local i32 @fclose(%struct._IO_FILE*) #1

; Function Attrs: nounwind
declare dso_local void @free(i8*) #2

attributes #0 = { noinline nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 7.0.0 (tags/RELEASE_700/final)"}
!2 = !{i8* %call3}
!3 = !{i8* %arrayidx5}
!4 = !{i8* %arrayidx7}
!5 = !{i8 %1}
!6 = !{i32 %conv}
!7 = !{i32 %2}
```

the !taint metadata is present at `ret i32 %2, !taint !7`, and it indicates the operand `%2` is tainted before this instruction executed.

### TODO

Implement taint propagation based on IFDS analysis.