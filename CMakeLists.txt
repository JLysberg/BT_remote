# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(peripheral)

target_sources(app PRIVATE
  src/DS3231.c
  src/lcs.c
  src/cts.c
  src/als.c
  src/main.c
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 
