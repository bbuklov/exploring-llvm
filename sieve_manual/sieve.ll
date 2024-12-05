@.str_array_size = private unnamed_addr constant [46 x i8] c"Initializing array with size of: %llu bytes \0A\00", align 1
@.str_val = private unnamed_addr constant [7 x i8] c"%llu \0A\00", align 1
@.str_newline = private unnamed_addr constant [2 x i8] c"\0A\00", align 1
@.str_alloc_fail = private unnamed_addr constant [28 x i8] c"Unable to allocate memory.\0A\00", align 1
@.str_wrong_input = private unnamed_addr constant [37 x i8] c"Given value must be greater then 1.\0A\00", align 1

declare i64 @printf(ptr, ...)
declare i64 @atoi(ptr)
declare ptr @malloc(i64)
declare void @free(ptr)
declare void @exit(i64)

define void @set_bit(i64 %byte_idx, i64 %bit_idx, i8* %primes, i1 %val) {
entry:
  %byte_ptr = getelementptr i8, i8* %primes, i64 %byte_idx
  %byte_val = load i8, i8* %byte_ptr
  %bit_idx_cast = trunc i64 %bit_idx to i8
  %mask = shl i8 1, %bit_idx_cast
  %cmp = icmp eq i1 %val, 1
  br i1 %cmp, label %set_to_1, label %set_to_0

set_to_1:
  %new_byte_1 = or i8 %byte_val, %mask
  store i8 %new_byte_1, i8* %byte_ptr
  br label %return
  
set_to_0:
  %notmask = xor i8 %mask, -1
  %new_byte_0 = and i8 %byte_val, %notmask
  store i8 %new_byte_0, i8* %byte_ptr
  br label %return

return:
  ret void
}

define i1 @check_bit(i64 %byte_idx, i64 %bit_idx, i8* %primes) {
entry:
  %byte_ptr = getelementptr i8, i8* %primes, i64 %byte_idx
  %byte_val = load i8, i8* %byte_ptr
  %bit_idx_cast = trunc i64 %bit_idx to i8
  %mask = shl i8 1, %bit_idx_cast
  %result = and i8 %byte_val, %mask
  %result_shifted = lshr i8 %result, %bit_idx_cast
  %is_set = icmp eq i8 %result_shifted, 1

  ret i1 %is_set
}

define i8* @init_array(i64 %n) {
entry:
  %i = alloca i64, align 4
  %size_in_bytes_1 = udiv i64 %n, 8
  %size = add i64 %size_in_bytes_1, 1
  call i64 (ptr, ...) @printf(ptr @.str_array_size, i64 %size)
  %primes = call i8* @malloc(i64 %size)

  %alloc_check = icmp eq i8* %primes, null
  br i1 %alloc_check, label %alloc_fail, label %init

alloc_fail:
  call i64 @printf(ptr @.str_alloc_fail)
  call void @exit(i64 -1)
  unreachable

init:
  %cmp1 = icmp sgt i64 %n, 0
  br i1 %cmp1, label %init0, label %init1

init0:
  call void @set_bit(i64 0, i64 0, i8* %primes, i1 0)
  br label %init1

init1:
  %cmp2 = icmp sgt i64 %n, 1
  br i1 %cmp2, label %init2, label %init_loop

init2:
  call void @set_bit(i64 0, i64 1, i8* %primes, i1 0)
  br label %init_loop

init_loop:
  store i64 2, i64* %i
  br label %init_loop_cond

init_loop_cond:
  %i_val = load i64, i64* %i
  %cmp3 = icmp slt i64 %i_val, %n
  br i1 %cmp3, label %init_loop_body, label %return

init_loop_body:
  %byte_idx = udiv i64 %i_val, 8
  %bit_idx = urem i64 %i_val, 8
  call void @set_bit(i64 %byte_idx, i64 %bit_idx, i8* %primes, i1 1)
  %i_next = add i64 %i_val, 1
  store i64 %i_next, i64* %i
  br label %init_loop_cond

return:
  ret i8* %primes
}

define void @sieve(i64 %prime, i8* %primes, i64 %upper_bound) {
entry:
  %i = alloca i64, align 4
  store i64 2, i64* %i
  %real_upper_bound_1 = udiv i64 %upper_bound, %prime
  %real_upper_bound = add i64 %real_upper_bound_1, 1
  br label %sieve_loop_cond

sieve_loop_cond:
  %i_val = load i64, ptr %i
  %cmp1 = icmp slt i64 %i_val, %real_upper_bound
  br i1 %cmp1, label %sieve_loop_body, label %sieve_done

sieve_loop_body:
  %product = mul i64 %i_val, %prime
  %byte_idx = udiv i64 %product, 8
  %bit_idx = urem i64 %product, 8
  call void @set_bit(i64 %byte_idx, i64 %bit_idx, i8* %primes, i1 0)
  %i_next = add i64 %i_val, 1
  store i64 %i_next, i64* %i
  br label %sieve_loop_cond

sieve_done:
  ret void
}

define i64 @next_prime(i64 %prime, i8* %primes, i64 %upper_bound) {
entry:
  %i = alloca i64, align 4
  %next_prime = alloca i64, align 4
  %start_index = add i64 %prime, 1
  store i64 %start_index, i64* %i
  store i64 %prime, i64* %next_prime
  br label %next_prime_loop_cond

next_prime_loop_cond:
  %i_val = load i64, i64* %i
  %cmp1 = icmp slt i64 %i_val, %upper_bound
  br i1 %cmp1, label %next_prime_loop_body, label %next_prime_not_found

next_prime_loop_body:
  %byte_idx = udiv i64 %i_val, 8
  %bit_idx = urem i64 %i_val, 8
  %is_set = call i1 @check_bit(i64 %byte_idx, i64 %bit_idx, i8* %primes)
  br i1 %is_set, label %next_prime_found, label %next_prime_iter

next_prime_iter:
  %i_next = add i64 %i_val, 1
  store i64 %i_next, i64* %i
  br label %next_prime_loop_cond

next_prime_not_found:
  %next_prime_val = load i64, i64* %next_prime
  ret i64 %next_prime_val

next_prime_found:
  ret i64 %i_val
}

define void @calc_primes(i64 %n, i8* %primes) {
entry:
  %i = alloca i64, align 4
  store i64 2, i64* %i
  br label %calc_primes_loop_cond

calc_primes_loop_cond:
  %i_val = load i64, ptr %i
  %cmp1 = icmp slt i64 %i_val, %n
  br i1 %cmp1, label %calc_primes_loop_body, label %calc_primes_done

calc_primes_loop_body:
  call i64 @printf(ptr @.str_val, i64 %i_val)
  %byte_idx = udiv i64 %i_val, 8
  %bit_idx = urem i64 %i_val, 8
  call void @sieve(i64 %i_val, i8* %primes, i64 %n)
  %next_prime = call i64 @next_prime(i64 %i_val, i8* %primes, i64 %n)
  store i64 %next_prime, i64* %i
  %cmp2 = icmp eq i64 %next_prime, %i_val
  br i1 %cmp2, label %calc_primes_done, label %calc_primes_loop_cond

calc_primes_done:
  ret void
}

define i64 @main(i64 %argc, ptr %argv) {
entry:
  %argc_check = icmp eq i64 %argc, 2
  br i1 %argc_check, label %parse_input, label %error

parse_input:
  %n_ptr = getelementptr ptr, ptr %argv, i64 1
  %n_arg = load ptr, ptr %n_ptr
  %n = call i64 @atoi(ptr %n_arg)
  %n_valid = icmp sgt i64 %n, 1
  br i1 %n_valid, label %main_body, label %error

main_body:
  %primes = call i8* @init_array(i64 %n)
  call void @calc_primes(i64 %n, i8* %primes)
  br label %cleanup

cleanup:
  call void @free(ptr %primes)
  ret i64 0

error:
  call i64 @printf(ptr @.str_wrong_input)
  call void @exit(i64 -1)
  unreachable
}
