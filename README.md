# 🚀 FPrime-OrbitSim

**Real-time Orbital Simulation using NASA's F Prime Framework (Raspberry Pi Hardware-in-the-Loop)**  
*A bridge between spacecraft dynamics theory and embedded flight software implementation.*

---

## 📌 Abstract

This project implements a hardware-in-the-loop (HIL) orbital simulation using **NASA’s F´ (F Prime)** flight software framework on **Raspberry Pi**, integrating:

- 📡 Keplerian orbital propagation  
- 🎯 Quaternion-based attitude kinematics  
- 🔍 Early-stage fault monitoring and recovery concepts  
- 💡 Low-level C++ implementation for real-time processing  
- 📐 Scientific validation against MATLAB models and academic feedback

✔ Developed as part of the **“Physics of Space Systems” (EN.615.744)** course at **Johns Hopkins University (JHU EP)**.  
✔ Serving as a stepping stone toward a **live demo at the NASA JPL F´ Workshop (planned 2026)**.

---

## 🔭 Project Objectives

| Goal                               | Status        |
|------------------------------------|---------------|
| Open-loop orbital propagation      | ✔ Completed   |
| Quaternion-based attitude update   | ✔ Implemented & Tested |
| Hardware deployment on Raspberry Pi| ✔ Operational |
| HIL test validation                | 🚧 In progress |
| Fault detection and telemetry      | 🚧 In progress |
| Closed-loop ADCS control           | 🔜 Phase 2    |
| JPL demo preparation               | 🔜 Q1 2026    |

---

## 🛰️ System Overview

At a high level, **OrbitSim** is intended to combine:

- Keplerian orbital propagation and perturbation models  
- Quaternion-based attitude kinematics and attitude update  
- Hardware-in-the-loop (HIL) execution on a Raspberry Pi target  
- Early-stage fault monitoring, telemetry, and recovery logic  
- Validation against MATLAB-based reference models and academic feedback  

Core dynamics, guidance, and control modules are planned to run as F´ components, while selected states and events are exposed to hardware interfaces (e.g., LEDs, sensors, and eventually actuators) for HIL demos and educational use.

---

## 🔌 Current Raspberry Pi Gateway (MorseBlinker)

In the current stage, this repository focuses on a concrete **hardware gateway** between the main OrbitSim software and a Raspberry Pi:

- Receives high-level text messages via F´ commands (e.g., `"sos"`).  
- Converts those messages into **Morse code timing** (dots, dashes, symbol/word gaps).  
- Drives **GPIO 17** on a Raspberry Pi to blink an LED according to the encoded Morse pattern.

This gateway is designed as a **bridge** between the core OrbitSim projects and physical hardware:

```text
OrbitSim core components
    (simulation, guidance, control, etc.)
           │
           │  F´ commands / events
           ▼
   MorseBlinker gateway (this project)
           │
           │  GPIO 17, digital on/off
           ▼
   Physical LED on Raspberry Pi
