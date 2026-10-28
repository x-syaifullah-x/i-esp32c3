# JSON Response Format Specification — Command Dispatch

This document defines the standard JSON response format used by the command dispatch system.

## Overview

Every response returned by the system follows this general structure:

```json
{
  "action": 0,
  "status": 0,
  "data": {}
}
```

## Fields

| Field    | Type                     | Description                                                                                                                           |
|----------|--------------------------|---------------------------------------------------------------------------------------------------------------------------------------|
| `action` | `uint8_t`                | Numeric code identifying the action/command executed                                                                                  |
| `status` | `uint8_t`                | Execution result. `0` = OK/success. Any value greater than `0` indicates an error, where the value itself is the specific error code. |
| `data`   | `array`\|`object`        | Content depends on the value of `status`                                                                                              |

## `data` Rules

- **`status = 0` (success)** — `data` contains the result of the executed command. It can be either an `array` or an `object`, depending on the command.
- **`status > 0` (error)** — `data` contains error details, following the error structure defined below. The numeric value of `status` itself is the error code.

## Error Data Structure

When `status > 0`, the `data` field is always an `object` with the following structure:

```json
{
  "message": "..."
}
```

| Field     | Type     | Description                                                                   |
|-----------|----------|-------------------------------------------------------------------------------|
| `message` | `string` | Short human-readable description of the error, useful for logging/debugging. The error code is already carried by `status`. |

## Examples

### Success — `data` as an object

```json
{
  "action": 1,
  "status": 0,
  "data": {
    "mode": "AP",
    "ssid": "MyNetwork"
  }
}
```

### Success — `data` as an array

```json
{
  "action": 2,
  "status": 0,
  "data": [
    { "mac": "AA:BB:CC:DD:EE:01", "rssi": -45 },
    { "mac": "AA:BB:CC:DD:EE:02", "rssi": -60 }
  ]
}
```

### Error

```json
{
  "action": 1,
  "status": 4,
  "data": {
    "message": "interface not found"
  }
}
```