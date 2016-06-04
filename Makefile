# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

# Default target executed when no arguments are given to make.
default_target: all
.PHONY : default_target

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/john/projects/johnsynth

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/john/projects/johnsynth

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running interactive CMake command-line interface..."
	/usr/bin/cmake -i .
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache
.PHONY : edit_cache/fast

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache
.PHONY : rebuild_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/john/projects/johnsynth/CMakeFiles /home/john/projects/johnsynth/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/john/projects/johnsynth/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean
.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named johnsynth

# Build rule for target.
johnsynth: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 johnsynth
.PHONY : johnsynth

# fast build rule for target.
johnsynth/fast:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/build
.PHONY : johnsynth/fast

bcr2000Driver.o: bcr2000Driver.cpp.o
.PHONY : bcr2000Driver.o

# target to build an object file
bcr2000Driver.cpp.o:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/bcr2000Driver.cpp.o
.PHONY : bcr2000Driver.cpp.o

bcr2000Driver.i: bcr2000Driver.cpp.i
.PHONY : bcr2000Driver.i

# target to preprocess a source file
bcr2000Driver.cpp.i:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/bcr2000Driver.cpp.i
.PHONY : bcr2000Driver.cpp.i

bcr2000Driver.s: bcr2000Driver.cpp.s
.PHONY : bcr2000Driver.s

# target to generate assembly for a file
bcr2000Driver.cpp.s:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/bcr2000Driver.cpp.s
.PHONY : bcr2000Driver.cpp.s

buffer.o: buffer.cpp.o
.PHONY : buffer.o

# target to build an object file
buffer.cpp.o:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/buffer.cpp.o
.PHONY : buffer.cpp.o

buffer.i: buffer.cpp.i
.PHONY : buffer.i

# target to preprocess a source file
buffer.cpp.i:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/buffer.cpp.i
.PHONY : buffer.cpp.i

buffer.s: buffer.cpp.s
.PHONY : buffer.s

# target to generate assembly for a file
buffer.cpp.s:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/buffer.cpp.s
.PHONY : buffer.cpp.s

generateTone.o: generateTone.cpp.o
.PHONY : generateTone.o

# target to build an object file
generateTone.cpp.o:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/generateTone.cpp.o
.PHONY : generateTone.cpp.o

generateTone.i: generateTone.cpp.i
.PHONY : generateTone.i

# target to preprocess a source file
generateTone.cpp.i:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/generateTone.cpp.i
.PHONY : generateTone.cpp.i

generateTone.s: generateTone.cpp.s
.PHONY : generateTone.s

# target to generate assembly for a file
generateTone.cpp.s:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/generateTone.cpp.s
.PHONY : generateTone.cpp.s

main.o: main.cpp.o
.PHONY : main.o

# target to build an object file
main.cpp.o:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/main.cpp.o
.PHONY : main.cpp.o

main.i: main.cpp.i
.PHONY : main.i

# target to preprocess a source file
main.cpp.i:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/main.cpp.i
.PHONY : main.cpp.i

main.s: main.cpp.s
.PHONY : main.s

# target to generate assembly for a file
main.cpp.s:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/main.cpp.s
.PHONY : main.cpp.s

synthesiser.o: synthesiser.cpp.o
.PHONY : synthesiser.o

# target to build an object file
synthesiser.cpp.o:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/synthesiser.cpp.o
.PHONY : synthesiser.cpp.o

synthesiser.i: synthesiser.cpp.i
.PHONY : synthesiser.i

# target to preprocess a source file
synthesiser.cpp.i:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/synthesiser.cpp.i
.PHONY : synthesiser.cpp.i

synthesiser.s: synthesiser.cpp.s
.PHONY : synthesiser.s

# target to generate assembly for a file
synthesiser.cpp.s:
	$(MAKE) -f CMakeFiles/johnsynth.dir/build.make CMakeFiles/johnsynth.dir/synthesiser.cpp.s
.PHONY : synthesiser.cpp.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... edit_cache"
	@echo "... johnsynth"
	@echo "... rebuild_cache"
	@echo "... bcr2000Driver.o"
	@echo "... bcr2000Driver.i"
	@echo "... bcr2000Driver.s"
	@echo "... buffer.o"
	@echo "... buffer.i"
	@echo "... buffer.s"
	@echo "... generateTone.o"
	@echo "... generateTone.i"
	@echo "... generateTone.s"
	@echo "... main.o"
	@echo "... main.i"
	@echo "... main.s"
	@echo "... synthesiser.o"
	@echo "... synthesiser.i"
	@echo "... synthesiser.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

