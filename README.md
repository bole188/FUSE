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

## Introduction

The goal of this project is to create a filesystem that mirrors the structure of TTConnect Wave devices and their associated sub-devices. By leveraging FUSE, we can implement this filesystem in user space, allowing for flexibility and ease of development. This approach is inspired by examples from the libfuse library and other FUSE-based filesystems.

## Device Structure

The filesystem represents devices using directories and files, each with specific attributes and behaviors.

### Fields Explanation

- **name**: The name of the file or directory.
- **model**: Specifies the device model, which can be one of the following:

  - **Electronic Control Units:**
    - HY-TTC_71
    - HY-TTC_60
    - HY-TTC_50
    - HY-TTC_30
    - HY-TTC_508
    - HY-TTC_94

  - **I/O Modules:**
    - HY-TTC_48

  - **Rugged Operator Interfaces:**
    - VISION_3
    - VISION_305

  - **Connectivity Solutions:**
    - TTConnectWave

  - **Others:**
    - ACTUATOR
    - SENSOR

- **serial number**: The hardware's serial number.
- **registration date**: The Unix timestamp representing when the device was registered in the cloud, corresponding to the creation time of the file or directory.
- **system id**: A randomly generated seven-digit identification number within the system.
- **imei**: The unique identifier of the mobile device.


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

