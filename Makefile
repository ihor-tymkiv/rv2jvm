CC = gcc
JAVA = java
JAVAC = javac

BUILD_DIR = build
JAVA_SRC_DIR = java/src/main/java
PROGRAM ?= examples/test.s

all: build

build: $(BUILD_DIR)/rv2jvm.exe $(BUILD_DIR)/rv2jvm/Runner.class

$(BUILD_DIR)/rv2jvm.exe: rv2jvm/src/*.c
	mkdir -p $(BUILD_DIR)
	$(CC) $^ -o $(BUILD_DIR)/rv2jvm.exe

$(BUILD_DIR)/rv2jvm/RvRuntime.class: $(BUILD_DIR)/rv2jvm.exe $(PROGRAM)
	mkdir -p $(BUILD_DIR)/rv2jvm
	cd $(BUILD_DIR)/rv2jvm && ../rv2jvm.exe $(abspath $(PROGRAM))

$(BUILD_DIR)/rv2jvm/Runner.class: $(JAVA_SRC_DIR)/rv2jvm/Runner.java \
								  $(BUILD_DIR)/rv2jvm/RvRuntime.class
	mkdir -p $(BUILD_DIR)
	$(JAVAC) -d $(BUILD_DIR) -cp $(BUILD_DIR) $<

run: build $(BUILD_DIR)/rv2jvm/RvRuntime.class
	$(JAVA) -cp $(BUILD_DIR) rv2jvm.Runner

clean:
	rm -rf $(BUILD_DIR)
