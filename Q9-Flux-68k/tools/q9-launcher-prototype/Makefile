CXX      ?= c++
CXXFLAGS := -std=c++14 -Wall -Wextra -Werror -O2
TVISION  := ../../third_party/tvision
BUILD    := build

all: $(BUILD)/q9-launcher

$(BUILD)/q9-launcher: src/q9_launcher.cpp src/launcher_model.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -Isrc -I$(TVISION)/include $< $(TVISION)/build/libtvision.a -lncurses -o $@

test: $(BUILD)/launcher-model-test
	$(BUILD)/launcher-model-test

$(BUILD)/launcher-model-test: test/launcher_model_test.cpp src/launcher_model.h
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -Isrc $< -o $@

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
