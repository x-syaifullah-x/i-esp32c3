.PHONY: $(MAKECMDGOALS)

SHELL := /bin/bash

PROJECT_NAME := $(notdir $(CURDIR))
ROOT_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
BUILD_DIR = $(ROOT_DIR)/build

dependencies:
	apt install --no-install-suggests --no-install-recommends -y cmake ninja-build

configure:
	cmake -S $(ROOT_DIR) -B $(BUILD_DIR)

build:
	cmake --build $(BUILD_DIR)

clean:
	rm -rfv $(BUILD_DIR)