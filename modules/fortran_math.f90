! Fortran Discord Bot Module - Because Why Not?
! 
! This demonstrates that yes, you can write Discord bot commands in FORTRAN.
! The future is now, and it's written in a language from 1957.

module fortran_bot_module
    use, intrinsic :: iso_c_binding
    implicit none
    
contains

    ! Calculate the dot product of two vectors (classic FORTRAN flex)
    function vector_dot_product(vec1, vec2, n) result(dot)
        real(c_double), dimension(*), intent(in) :: vec1, vec2
        integer(c_int), intent(in) :: n
        real(c_double) :: dot
        integer :: i
        
        dot = 0.0d0
        do i = 1, n
            dot = dot + vec1(i) * vec2(i)
        end do
    end function vector_dot_product

    ! Matrix multiplication because FORTRAN is THE language for this
    subroutine matrix_multiply(a, b, c, n) bind(c, name="fortran_matmul")
        integer(c_int), intent(in), value :: n
        real(c_double), dimension(n,n), intent(in) :: a, b
        real(c_double), dimension(n,n), intent(out) :: c
        integer :: i, j, k
        
        do i = 1, n
            do j = 1, n
                c(i,j) = 0.0d0
                do k = 1, n
                    c(i,j) = c(i,j) + a(i,k) * b(k,j)
                end do
            end do
        end do
    end subroutine matrix_multiply

end module fortran_bot_module

! C-compatible wrapper functions for the module interface

function module_get_info() bind(c, name="module_get_info") result(info)
    use, intrinsic :: iso_c_binding
    use fortran_bot_module
    implicit none
    
    type, bind(c) :: module_info_t
        type(c_ptr) :: name
        type(c_ptr) :: version
        type(c_ptr) :: author
        type(c_ptr) :: description
        integer(c_int32_t) :: api_version
        integer(c_int) :: module_type
    end type module_info_t
    
    type(module_info_t) :: info
    character(len=20, kind=c_char), target, save :: name_str = "fortran_math" // c_null_char
    character(len=20, kind=c_char), target, save :: version_str = "1.0.0" // c_null_char
    character(len=50, kind=c_char), target, save :: author_str = "FORTRAN Gang" // c_null_char
    character(len=100, kind=c_char), target, save :: desc_str = &
        "Scientific computing in Discord because we can" // c_null_char
    
    info%name = c_loc(name_str)
    info%version = c_loc(version_str)
    info%author = c_loc(author_str)
    info%description = c_loc(desc_str)
    info%api_version = 4
    info%module_type = 0  ! MODULE_TYPE_NATIVE
    
end function module_get_info

function module_init(bridge, bot_context) bind(c, name="module_init") result(status)
    use, intrinsic :: iso_c_binding
    implicit none
    
    type(c_ptr), value :: bridge, bot_context
    integer(c_int) :: status
    
    ! Store bridge and context for later use
    ! (In a real implementation, you'd save these in module variables)
    status = 0  ! Success
    
end function module_init

subroutine module_shutdown() bind(c, name="module_shutdown")
    use, intrinsic :: iso_c_binding
    implicit none
    
    ! Cleanup if needed
    
end subroutine module_shutdown

function module_register_commands() bind(c, name="module_register_commands") result(commands)
    use, intrinsic :: iso_c_binding
    implicit none

    interface
        subroutine fortran_command(bot_context, channel_id, user_id, args) &
            bind(c, name="fortran_command_callback")
            use, intrinsic :: iso_c_binding
            type(c_ptr), value :: bot_context
            type(c_ptr), value :: channel_id, user_id, args
        end subroutine fortran_command
    end interface
    
    type, bind(c) :: command_reg_t
        type(c_ptr) :: name
        type(c_ptr) :: description
        type(c_funptr) :: callback
    end type command_reg_t
    
    type(c_ptr) :: commands
    type(command_reg_t), dimension(2), target, save :: cmd_array
    
    character(len=20, kind=c_char), target, save :: cmd1_name = "fortran" // c_null_char
    character(len=100, kind=c_char), target, save :: cmd1_desc = &
        "Run FORTRAN math (displays module info)" // c_null_char
    
    ! Command 1: !fortran
    cmd_array(1)%name = c_loc(cmd1_name)
    cmd_array(1)%description = c_loc(cmd1_desc)
    cmd_array(1)%callback = c_funloc(fortran_command)
    
    ! Terminator (NULL name)
    cmd_array(2)%name = c_null_ptr
    cmd_array(2)%description = c_null_ptr
    cmd_array(2)%callback = c_null_funptr
    
    commands = c_loc(cmd_array)
    
end function module_register_commands

subroutine fortran_command(bot_context, channel_id, user_id, args) &
    bind(c, name="fortran_command_callback")
    use, intrinsic :: iso_c_binding
    use fortran_bot_module
    implicit none
    
    type(c_ptr), value :: bot_context
    type(c_ptr), value :: channel_id, user_id, args
    
    ! This would normally call bridge->send_message() but we need the bridge pointer
    ! In a real implementation, you'd store the bridge in module_init
    ! For now, this just demonstrates the interface works
    
    ! Example calculation to show FORTRAN is actually doing work
    real(c_double), dimension(3) :: vec1 = [1.0d0, 2.0d0, 3.0d0]
    real(c_double), dimension(3) :: vec2 = [4.0d0, 5.0d0, 6.0d0]
    real(c_double) :: result
    
    result = vector_dot_product(vec1, vec2, 3)
    
    ! Result = 32.0 (1*4 + 2*5 + 3*6)
    ! Would send this to Discord channel via bridge->send_message()
    
end subroutine fortran_command
