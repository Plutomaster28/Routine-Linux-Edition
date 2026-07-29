# Writing Discord Bot Modules in FORTRAN

Because sometimes you need that 1957 energy in your 2025 Discord bot.

## Why FORTRAN?

- **Raw Performance**: FORTRAN is still THE language for numerical computing
- **Unholy Combination**: When Python bots aren't cursed enough
- **Scientific Computing**: Need FFT in your Discord bot? We got you
- **Flex Factor**: Maximum disrespect to conventional bot development

## Requirements

You'll need a FORTRAN compiler:
- **Windows**: Install MinGW-w64 with gfortran
- **Linux**: `sudo apt install gfortran` or `sudo dnf install gcc-gfortran`
- **macOS**: `brew install gcc` (includes gfortran)

## Writing a FORTRAN Module

### 1. Use ISO_C_BINDING

FORTRAN's `ISO_C_BINDING` module provides C interoperability:

```fortran
module my_fortran_module
    use, intrinsic :: iso_c_binding
    implicit none
contains
    ! Your FORTRAN code here
end module
```

### 2. Implement Required Exports

All modules must export these C-compatible functions:

```fortran
! Module information
function module_get_info() bind(c, name="module_get_info") result(info)
    use, intrinsic :: iso_c_binding
    ! ... implementation
end function

! Initialize
function module_init(bridge, bot_context) bind(c, name="module_init") result(status)
    ! ... implementation
end function

! Shutdown
subroutine module_shutdown() bind(c, name="module_shutdown")
    ! ... implementation
end subroutine

! Register commands
function module_register_commands() bind(c, name="module_register_commands") result(commands)
    ! ... implementation
end function
```

### 3. C Type Mappings

| C Type | FORTRAN Type |
|--------|--------------|
| `int` | `integer(c_int)` |
| `uint32_t` | `integer(c_int32_t)` |
| `uint64_t` | `integer(c_int64_t)` |
| `double` | `real(c_double)` |
| `char*` | `type(c_ptr)` + `c_f_pointer` |
| `void*` | `type(c_ptr)` |
| function pointer | `type(c_funptr)` + `c_funloc` |

### 4. String Handling

FORTRAN strings need special care:

```fortran
! C string to FORTRAN
character(len=100), pointer :: fortran_str
type(c_ptr) :: c_str_ptr

call c_f_pointer(c_str_ptr, fortran_str)

! FORTRAN string to C (must be NULL terminated and static/save)
character(len=50, kind=c_char), target, save :: my_string = &
    "Hello from FORTRAN" // c_null_char
type(c_ptr) :: ptr_to_string

ptr_to_string = c_loc(my_string)
```

### 5. Struct Mapping

```fortran
type, bind(c) :: module_info_t
    type(c_ptr) :: name
    type(c_ptr) :: version
    type(c_ptr) :: author
    type(c_ptr) :: description
    integer(c_int32_t) :: api_version
    integer(c_int) :: module_type
end type module_info_t
```

## Building

Just drop your `.f90` file in the `modules/` directory and run:

```bash
cd modules
cmake -S . -B build_modules
cmake --build build_modules
```

CMake will automatically:
1. Detect your FORTRAN file
2. Enable the Fortran language
3. Compile it to a `.dll` (Windows) or `.so` (Linux/macOS)
4. Link it with the module interface

## Example: Vector Math Module

See [fortran_math.f90](fortran_math.f90) for a complete example that implements:
- Vector dot product
- Matrix multiplication
- Discord command callbacks
- Full module interface

## What You Can Do

### Scientific Computing
- FFT and signal processing
- Matrix operations (BLAS-style)
- Numerical integration
- Statistics and probability

### Performance Critical Tasks
- Real-time audio processing
- High-throughput data processing
- Scientific simulations
- Legacy FORTRAN library integration

### Pure Chaos
- Reimplement parts of Discord.py in FORTRAN
- Create the world's first FORTRAN chatbot
- Make Python developers cry
- Assert dominance

## Tips

1. **Array Indexing**: FORTRAN uses 1-based indexing by default
2. **Column-Major**: FORTRAN stores arrays in column-major order (opposite of C)
3. **SAVE Attribute**: Use `save` for static variables in functions
4. **Implicit None**: Always use `implicit none` to catch typos
5. **Intent**: Specify `intent(in)`, `intent(out)`, or `intent(inout)` for clarity

## Common Pitfalls

### String Lifetime
```fortran
! ❌ WRONG - Local string, invalid after function returns
function get_name() result(ptr)
    character(len=20, kind=c_char), target :: name
    name = "MyName" // c_null_char
    ptr = c_loc(name)  ! DANGER: pointer to dead memory
end function

! ✅ CORRECT - Static string with SAVE
function get_name() result(ptr)
    character(len=20, kind=c_char), target, save :: name = &
        "MyName" // c_null_char
    ptr = c_loc(name)  ! Safe: static storage
end function
```

### Array Passing
```fortran
! C passes arrays as pointers, FORTRAN needs assumed-size
subroutine process_array(arr, n) bind(c)
    integer(c_int), value :: n
    real(c_double), dimension(*) :: arr  ! Assumed-size array
    ! Use arr(1) to arr(n)
end subroutine
```

## The Stack

Your unholy Discord bot now supports:

- **C**: Low-level control
- **C++**: Object-oriented structure  
- **Assembly**: Raw machine code
- **Lua**: Scripting and logic
- **FORTRAN**: Scientific computing and maximum disrespect

## Resources

- [FORTRAN ISO_C_BINDING Guide](https://gcc.gnu.org/onlinedocs/gfortran/ISO_005fC_005fBINDING.html)
- [Modern Fortran Tutorial](https://fortran-lang.org/learn/)
- Module Interface: [module_interface.h](../include/module_interface.h)

---

*"We were so preoccupied with whether we could, we didn't stop to think if we should."*
