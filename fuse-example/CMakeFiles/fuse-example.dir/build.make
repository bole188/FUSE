# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example

# Include any dependencies generated for this target.
include CMakeFiles/fuse-example.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/fuse-example.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/fuse-example.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/fuse-example.dir/flags.make

CMakeFiles/fuse-example.dir/src/fuse-example.c.o: CMakeFiles/fuse-example.dir/flags.make
CMakeFiles/fuse-example.dir/src/fuse-example.c.o: src/fuse-example.c
CMakeFiles/fuse-example.dir/src/fuse-example.c.o: CMakeFiles/fuse-example.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/fuse-example.dir/src/fuse-example.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/fuse-example.dir/src/fuse-example.c.o -MF CMakeFiles/fuse-example.dir/src/fuse-example.c.o.d -o CMakeFiles/fuse-example.dir/src/fuse-example.c.o -c /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/src/fuse-example.c

CMakeFiles/fuse-example.dir/src/fuse-example.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fuse-example.dir/src/fuse-example.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/src/fuse-example.c > CMakeFiles/fuse-example.dir/src/fuse-example.c.i

CMakeFiles/fuse-example.dir/src/fuse-example.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fuse-example.dir/src/fuse-example.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/src/fuse-example.c -o CMakeFiles/fuse-example.dir/src/fuse-example.c.s

CMakeFiles/fuse-example.dir/src/device_manager.c.o: CMakeFiles/fuse-example.dir/flags.make
CMakeFiles/fuse-example.dir/src/device_manager.c.o: src/device_manager.c
CMakeFiles/fuse-example.dir/src/device_manager.c.o: CMakeFiles/fuse-example.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/fuse-example.dir/src/device_manager.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/fuse-example.dir/src/device_manager.c.o -MF CMakeFiles/fuse-example.dir/src/device_manager.c.o.d -o CMakeFiles/fuse-example.dir/src/device_manager.c.o -c /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/src/device_manager.c

CMakeFiles/fuse-example.dir/src/device_manager.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/fuse-example.dir/src/device_manager.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/src/device_manager.c > CMakeFiles/fuse-example.dir/src/device_manager.c.i

CMakeFiles/fuse-example.dir/src/device_manager.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/fuse-example.dir/src/device_manager.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/src/device_manager.c -o CMakeFiles/fuse-example.dir/src/device_manager.c.s

# Object files for target fuse-example
fuse__example_OBJECTS = \
"CMakeFiles/fuse-example.dir/src/fuse-example.c.o" \
"CMakeFiles/fuse-example.dir/src/device_manager.c.o"

# External object files for target fuse-example
fuse__example_EXTERNAL_OBJECTS =

bin/fuse-example: CMakeFiles/fuse-example.dir/src/fuse-example.c.o
bin/fuse-example: CMakeFiles/fuse-example.dir/src/device_manager.c.o
bin/fuse-example: CMakeFiles/fuse-example.dir/build.make
bin/fuse-example: /usr/lib/x86_64-linux-gnu/libfuse.so
bin/fuse-example: CMakeFiles/fuse-example.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable bin/fuse-example"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/fuse-example.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/fuse-example.dir/build: bin/fuse-example
.PHONY : CMakeFiles/fuse-example.dir/build

CMakeFiles/fuse-example.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/fuse-example.dir/cmake_clean.cmake
.PHONY : CMakeFiles/fuse-example.dir/clean

CMakeFiles/fuse-example.dir/depend:
	cd /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example /home/boskobrankovic/RTOS/FUSE_project/anadolu_fs/fuse-example/CMakeFiles/fuse-example.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/fuse-example.dir/depend
