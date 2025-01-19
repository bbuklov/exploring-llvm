 define dso_local i32 @foo(i32 %0, i32 %1, i32 %2,
 i32 %3) {
    %5 = sub i32 %2, %3
    %6 = mul i32 %5, %1
    %7 = add i32 %1, %6
    %8 = mul i32 %2, %7
    %9 = add i32 %0, %8
    %10 = sub i32 %2, %3
    %11 = mul i32 %10, %1
    %12 = add i32 %9, %11
    ret i32 %12
 }