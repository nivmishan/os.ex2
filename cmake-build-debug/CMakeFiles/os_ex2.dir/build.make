# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.15

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /opt/clion-2019.3.5/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /opt/clion-2019.3.5/bin/cmake/linux/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/kali/university/os/os.ex2

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/kali/university/os/os.ex2/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/os_ex2.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/os_ex2.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/os_ex2.dir/flags.make

CMakeFiles/os_ex2.dir/main.cpp.o: CMakeFiles/os_ex2.dir/flags.make
CMakeFiles/os_ex2.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/kali/university/os/os.ex2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/os_ex2.dir/main.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/os_ex2.dir/main.cpp.o -c /home/kali/university/os/os.ex2/main.cpp

CMakeFiles/os_ex2.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/os_ex2.dir/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/kali/university/os/os.ex2/main.cpp > CMakeFiles/os_ex2.dir/main.cpp.i

CMakeFiles/os_ex2.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/os_ex2.dir/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/kali/university/os/os.ex2/main.cpp -o CMakeFiles/os_ex2.dir/main.cpp.s

CMakeFiles/os_ex2.dir/uthreads.cpp.o: CMakeFiles/os_ex2.dir/flags.make
CMakeFiles/os_ex2.dir/uthreads.cpp.o: ../uthreads.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/kali/university/os/os.ex2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/os_ex2.dir/uthreads.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/os_ex2.dir/uthreads.cpp.o -c /home/kali/university/os/os.ex2/uthreads.cpp

CMakeFiles/os_ex2.dir/uthreads.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/os_ex2.dir/uthreads.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/kali/university/os/os.ex2/uthreads.cpp > CMakeFiles/os_ex2.dir/uthreads.cpp.i

CMakeFiles/os_ex2.dir/uthreads.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/os_ex2.dir/uthreads.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/kali/university/os/os.ex2/uthreads.cpp -o CMakeFiles/os_ex2.dir/uthreads.cpp.s

# Object files for target os_ex2
os_ex2_OBJECTS = \
"CMakeFiles/os_ex2.dir/main.cpp.o" \
"CMakeFiles/os_ex2.dir/uthreads.cpp.o"

# External object files for target os_ex2
os_ex2_EXTERNAL_OBJECTS =

os_ex2: CMakeFiles/os_ex2.dir/main.cpp.o
os_ex2: CMakeFiles/os_ex2.dir/uthreads.cpp.o
os_ex2: CMakeFiles/os_ex2.dir/build.make
os_ex2: CMakeFiles/os_ex2.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/kali/university/os/os.ex2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable os_ex2"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/os_ex2.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/os_ex2.dir/build: os_ex2

.PHONY : CMakeFiles/os_ex2.dir/build

CMakeFiles/os_ex2.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/os_ex2.dir/cmake_clean.cmake
.PHONY : CMakeFiles/os_ex2.dir/clean

CMakeFiles/os_ex2.dir/depend:
	cd /home/kali/university/os/os.ex2/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/kali/university/os/os.ex2 /home/kali/university/os/os.ex2 /home/kali/university/os/os.ex2/cmake-build-debug /home/kali/university/os/os.ex2/cmake-build-debug /home/kali/university/os/os.ex2/cmake-build-debug/CMakeFiles/os_ex2.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/os_ex2.dir/depend

