# Wave Device Filesystem

This project implements a custom filesystem using FUSE (Filesystem in Userspace) to simulate the structure and behavior of TTConnect Wave devices and their sub-devices. The filesystem is designed to represent devices as directories and files, adhering to specific rules and behaviors as outlined below.

## Table of Contents
  * [Introduction](#introduction)
  * [Device Structure](#device-structure)
    * [Fields Explanation](#fields-explanation)
    * [Directory Rules](#directory-rules)
    * [File Rules](#file-rules)
  * [Filesystem Operations](#filesystem-operations)
    * [Creating Directories](#creating-directories)
    * [Creating Files](#creating-files)
    * [Reading and Writing Files](#reading-and-writing-files)
  * [Filesystem Persistence](#filesystem-persistence)
    * [JSON Structure](#json-structure)
    * [Log File](#log-file)
  * [Implementation Details](#implementation-details)
  * [Building and Running](#building-and-running)
  * [References](#references)

## Introduction

The goal of this project is to create a filesystem that mirrors the structure of TTConnect Wave devices and their associated sub-devices. By leveraging FUSE, we can implement this filesystem in user space, allowing for flexibility and ease of development. This approach is inspired by examples from the libfuse library and other FUSE-based filesystems.

## Device Structure

The filesystem represents devices using directories and files, each with specific attributes and behaviors.

### Fields Explanation

Each device contains fields stored as files, representing its state and configuration.

### Directory Rules

- Devices are represented as directories.
- Sub-devices exist as subdirectories within the parent device directory.

### File Rules

- Each file represents a specific attribute of the device.
- File contents must conform to predefined formats.

## Filesystem Operations

### Creating Directories

- Users can create new directories to represent additional devices.

### Creating Files

- Files store configuration settings and device status information.

### Reading and Writing Files

- Reading a file returns the stored information.
- Writing updates the relevant device parameter.

## Filesystem Persistence

All files and directories are structured in a JSON file to maintain persistence.

### JSON Structure

- The filesystem hierarchy is stored in a structured JSON format.

### Log File

- A log file records changes and operations performed on the filesystem.

## Implementation Details

The implementation is based on libfuse, handling operations such as directory creation, file manipulation, and persistence.

## Building and Running

### Compilation

```sh
gcc -o wavefs wavefs.c `pkg-config --cflags --libs fuse`
