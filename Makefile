# Default target: if you followed the macOS guide this will run the
# project's build.sh inside the `qnx-build` Docker container so the
# QNX Linux host tools are available. This prevents `make` from trying
# to invoke a host `qcc` on macOS where it isn't available.
.PHONY: default
default:
	docker exec -it qnx-build /home/$(USER)/qnxprojects/my-project/build.sh

# Deployment settings - update these for your RPi
TARGET_IP ?= 192.168.2.22
TARGET_USER ?= qnxuser
TARGET_PATH ?= /tmp

.PHONY: deploy upload run test deploy-root run-root
deploy: default
	@echo "Deploying to $(TARGET_USER)@$(TARGET_IP):$(TARGET_PATH)/"
	scp build/aarch64le-debug/rubiks-cube-solver $(TARGET_USER)@$(TARGET_IP):$(TARGET_PATH)/

# Deploy as root user
deploy-root: default
	@echo "Deploying as root to $(TARGET_IP):$(TARGET_PATH)/"
	scp build/aarch64le-debug/rubiks-cube-solver root@$(TARGET_IP):$(TARGET_PATH)/

upload: deploy

run:
	@echo "Running on target..."
	ssh $(TARGET_USER)@$(TARGET_IP) "chmod +x $(TARGET_PATH)/rubiks-cube-solver && $(TARGET_PATH)/rubiks-cube-solver"

# Run as root user
run-root:
	@echo "Running as root on target..."
	ssh root@$(TARGET_IP) "chmod +x $(TARGET_PATH)/rubiks-cube-solver && $(TARGET_PATH)/rubiks-cube-solver"

test: deploy run

#Build artifact type, possible values shared, static and exe
ARTIFACT_TYPE = exe
PROJECT_NAME = rubiks-cube-solver

LDFLAGS_shared = -shared -o
ARTIFACT_NAME_shared = lib$(PROJECT_NAME).so

LDFLAGS_static = -static -a
ARTIFACT_NAME_static = lib$(PROJECT_NAME).a

LDFLAGS_exe = -o
ARTIFACT_NAME_exe = $(PROJECT_NAME)

ARTIFACT = $(ARTIFACT_NAME_$(ARTIFACT_TYPE))

#Build architecture/variant string, possible values: x86, armv7le, etc...
PLATFORM ?= x86_64

#Build profile, possible values: release, debug, profile, coverage
BUILD_PROFILE ?= debug

CONFIG_NAME ?= $(PLATFORM)-$(BUILD_PROFILE)
OUTPUT_DIR = build/$(CONFIG_NAME)
TARGET = $(OUTPUT_DIR)/$(ARTIFACT)

#Compiler definitions

CC = qcc -Vgcc_nto$(PLATFORM)
CXX = q++ -Vgcc_nto$(PLATFORM)_cxx

LD = $(CXX)

#User defined include/preprocessor flags and libraries

INCLUDES += -I./include
INCLUDES += -I$(QNX_TARGET)/aarch64le/usr/local/include/SDL2
INCLUDES += -I$(QNX_TARGET)/usr/include/opencv4

LIBS += -L$(QNX_TARGET)/aarch64le/usr/local/lib -lSDL2 -lSDL2main -lscreen -lm -lstdc++
LIBS += -L$(QNX_TARGET)/aarch64le/usr/lib -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_videoio

#Compiler flags for build profiles
CCFLAGS_release += -O2
CCFLAGS_debug += -g -O0 -fno-builtin
CCFLAGS_coverage += -g -O0 -ftest-coverage -fprofile-arcs
LDFLAGS_coverage += -ftest-coverage -fprofile-arcs
CCFLAGS_profile += -g -O0 -finstrument-functions
LIBS_profile += -lprofilingS

#Generic compiler flags (which include build type flags)
CCFLAGS_all += -Wall -fmessage-length=0 -fPIC
CCFLAGS_all += $(CCFLAGS_$(BUILD_PROFILE))

LDFLAGS_all += $(LDFLAGS_$(BUILD_PROFILE))
LIBS_all += $(LIBS_$(BUILD_PROFILE))
DEPS = -Wp,-MMD,$(@:%.o=%.d),-MT,$@

#Macro to expand files recursively: parameters $1 -  directory, $2 - extension, i.e. cpp
rwildcard = $(wildcard $(addprefix $1/*.,$2)) $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2))

#Source list
SRCS = $(call rwildcard, src, cpp)
SRCS += $(filter-out src/main.c, $(call rwildcard, src, c))

#Object files list
OBJS = $(addprefix $(OUTPUT_DIR)/,$(addsuffix .o, $(basename $(SRCS))))

#Compiling rule for c
$(OUTPUT_DIR)/%.o: %.c
	-@mkdir -p $(dir $@)
	$(CC) -c $(DEPS) -o $@ $(INCLUDES) $(CCFLAGS_all) $(CCFLAGS) $<

#Compiling rule for c++
$(OUTPUT_DIR)/%.o: %.cpp
	-@mkdir -p $(dir $@)
	$(CXX) -c $(DEPS) -o $@ $(INCLUDES) $(CCFLAGS_all) $(CCFLAGS) $<

#Linking rule
$(TARGET):$(OBJS)
	$(LD) $(LDFLAGS_$(ARTIFACT_TYPE)) $(TARGET) $(LDFLAGS_all) $(LDFLAGS) $(OBJS) $(LIBS_all) $(LIBS)

#Rules section for default compilation and linking
all: $(TARGET)

CLEAN_DIRS := $(shell find build -type d)
CLEAN_PATTERNS := *.o *.d $(ARTIFACT_NAME_exe) $(ARTIFACT_NAME_shared) $(ARTIFACT_NAME_static)
CLEAN_FILES := $(foreach DIR,$(CLEAN_DIRS),$(addprefix $(DIR)/,$(CLEAN_PATTERNS)))

clean:
	rm -f $(CLEAN_FILES)

rebuild: clean all

#Inclusion of dependencies (object files to source and includes)
-include $(OBJS:%.o=%.d)