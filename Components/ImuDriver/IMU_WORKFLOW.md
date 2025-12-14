# IMU Driver Component - Workflow & Architecture

## Overview
The `ImuDriver` is a core component of the OrbitSim project designed to simulate an Inertial Measurement Unit (IMU) within the F' (F Prime) framework. It provides real-time simulation of accelerometer and gyroscope data, numerical integration for position/velocity, and a strict state machine for fault-tolerant operation.

This component is designed with **Hardware-in-the-Loop (HIL)** capabilities in mind, utilizing a `MorseBlinker` component to provide physical feedback via an LED/Buzzer based on the internal system health.

## 1. System State Machine
The driver operates based on a strict 4-state workflow to ensure data integrity and prevent race conditions.

| State | Description | Transition Constraints |
| :--- | :--- | :--- |
| **NEVER_STARTED** | System initialization state. No data sampling. | Can only transition to `RUNNING`. |
| **RUNNING** | Active sampling and integration loop. | Can transition to `STOPPED`. |
| **STOPPED** | Sampling halted. System state frozen. | Can transition to `RUNNING` (Resume) or `RESET`. |
| **RESET** | Data cleared. Ready for fresh start. | Can transition to `RUNNING`. |

### Constraint Logic
* **Duplicate Command Protection:** Sending `IMU_START` while already running triggers a specific Telemetry Warning (`ALREADY_STARTED`) but **does not** reset the LED status or interrupt the current workflow.
* **Reset Protection:** `IMU_RESET` can only be executed if the system is first `STOPPED`. Attempting to reset while running triggers a `NEED_STOP_BEFORE_RESET` error.

---

## 2. Command Interface & API

### `IMU_START`
* **Behavior:** Starts the 50Hz scheduling loop.
* **Resume Logic:** If the system was previously stopped (and has data), it logs a "RESUMING" event; otherwise, it logs "STARTED".
* **Feedback:** Triggers LED to blink **'T'** (Tracking/True).

### `IMU_STOP`
* **Behavior:** Freezes the simulation.
* **Feedback:** Triggers LED to blink **'F'** (False/Frozen).
* **Error Handling:** If sent while already stopped, reports `ALREADY_STOPPED` without changing LED state.

### `IMU_RESET`
* **Pre-requisite:** System must be `STOPPED`.
* **Behavior:** Zeros out all vectors ($v, p, a, \omega$) and sample counters.
* **Feedback:** Logs `ImuReset` event.

### `IMU_SET_CONFIG`
Accepts a JSON string payload to dynamically tune simulation parameters.
* **Payload Example:** `{"dt": 0.01, "accelNoiseThresh": 20.0, "gyroNoiseThresh": 1.0}`
* **System Requirement:** F' Config `FW_CMD_STRING_MAX_SIZE` must be set to **256** to support long JSON strings.

---

## 3. Strict JSON Configuration Parser
The component implements a custom, robust JSON parser with the following validation rules:

1.  **Strict Key Validation:** Only allows defined keys (`dt`, `accelNoiseThresh`, `gyroNoiseThresh`). Unknown keys (e.g., `"aa":1`) trigger a `FORMAT_ERROR`.
2.  **Duplicate Key Prevention:** Passing the same key twice (e.g., `{"dt":0.1, "dt":0.2}`) triggers a `FORMAT_ERROR`.
3.  **Whitespace Agnostic:** Supports minified JSON or pretty-printed JSON with newlines/tabs.
4.  **Partial Updates:** Supports updating single parameters.

### Error Handling Policies

| Scenario | Result Code | System Action | LED Feedback |
| :--- | :--- | :--- | :--- |
| **Format Error** (Bad JSON, unknown key) | `FORMAT_ERROR` (3) | **STOP** System | Blinks **'E'** (Error) |
| **Range Warning** (Values out of bounds) | `VALIDATION_ERROR` (2) | **Continue** Running | Blinks **'W'** (Warning) |
| **Valid Config** | `OK` (0) | Update Params | Blinks **'T'** |

*Note: Range limits are `dt` (0.0-0.1s), `accelNoiseThresh` (0-100), `gyroNoiseThresh` (0-10).*

---

## 4. Computational Model (Physics)
The driver runs a numerical integration simulation at every scheduler tick:

### Inputs
* **Time Step ($dt$):** Configurable (default 0.01s).
* **Simulated Signals:** Sine wave generation for testing sensor drift.
    * $a_{x,y} = A \cdot \sin(t)$
    * $\omega_{x,y} = B \cdot \sin(0.5t)$

### Integration (Euler Method)
$$v_{new} = v_{old} + a \cdot dt$$
$$p_{new} = p_{old} + v_{new} \cdot dt$$

### Noise Detection
Calculates the Euclidean Norm of sensor vectors:
$$||a|| = \sqrt{a_x^2 + a_y^2 + a_z^2}$$
If $||a|| > Threshold_{accel}$, a **Noise Flag** is raised, setting the system Health to WARNING (LED 'W').

---

## 5. Deployment Requirements
To support standard JSON payloads, the F Prime configuration header must be overridden.

**File:** `config/FpConfig.h`
```c
// Increased from default (40) to support full JSON payloads
#define FW_CMD_STRING_MAX_SIZE 256