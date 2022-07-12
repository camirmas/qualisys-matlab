mex('-v', ...
    ['-L' './qualisys_cpp_sdk/build'], ... % specify build dir
    ['-l' 'qualisys_cpp_sdk'], ... % specify library (.so) file
    ['-I' './qualisys_cpp_sdk'], ... % specify where to search for #include files
    'sfun_mocap.cpp'); % specify S Function file