if has("unix")
    set makeprg=make

    let s:proj_root     = expand("<sfile>:p:h")
    let s:log_file      = s:proj_root."/make.log"
    let s:make_action   = "linux-debug64"

    " Build
    let s:make_command  = "make ".s:proj_root." ".s:make_action
    let s:build_action  = "!".s:proj_root."/.makebg.sh ".v:servername." \"".s:make_command."\" ".s:log_file

    " Execute
    let s:runtime_dir = s:proj_root."/runtime"
    let s:exec_action = "!../_build/linux64_gcc/bin/cmftDebug"

    function! Build()
        let curr_dir = getcwd()
        exec 'cd' s:proj_root
        exec s:build_action
        exec 'cd' curr_dir
    endfunc

    function! Execute()
        let curr_dir = getcwd()
        exec 'cd' s:runtime_dir
        exec s:exec_action
        exec 'cd' curr_dir
    endfunc

    nmap ,rr :call Build()<cr><cr>
    nmap ,ee :call Execute()<cr>

endif
