# STM32 CAN Intrusion Detection System (CAN IDS)

## Overview

This project implements a real-time CAN Bus Intrusion Detection System (IDS) using two STM32 microcontrollers communicating over the CAN bus.

The transmitter node periodically sends simulated vehicle data such as RPM, Speed and Temperature using FreeRTOS tasks. It can also generate different attack frames to emulate malicious CAN traffic.

The receiver node monitors all CAN traffic, detects suspicious messages, and generates real-time alerts using UART, LED and Buzzer.

The project demonstrates interrupt-driven CAN communication, FreeRTOS task scheduling, inter-task communication using queues, and basic intrusion detection techniques used in automotive embedded systems.

---

## Hardware Used

### Node 1 (CAN Message Generator)

- STM32F103C8T6 (Blue Pill)
- SN65HVD230 CAN Transceiver

### Node 2 (CAN IDS Receiver)

- STM32F407 Discovery
- SN65HVD230 CAN Transceiver

### Other Components

- SN65HVD230 CAN Bus
- Push Button
- Active Buzzer
- CAN Bus with 120 Ω termination resistors
- ST-Link V2

---

## Software Used

- STM32CubeIDE
- STM32CubeMX
- FreeRTOS
- HAL Drivers
- Tera Term

---

# Features

- CAN Bus Communication
- FreeRTOS on both STM32 boards
- Interrupt-driven CAN Reception
- Queue-based Task Communication
- UART Logging
- Red LED Alert
- Buzzer Alert

Implemented attack detection:

- Unknown CAN ID Detection
- Invalid RPM Payload Detection
- CAN Flood Detection

---

# FreeRTOS Architecture

## Node 1 (CAN Generator)

### RPM Task

Period: 10 ms

Generates RPM CAN frame.

---

### Speed Task

Period: 20 ms

Generates Speed CAN frame.

---

### Temperature Task

Period: 100 ms

Generates Temperature CAN frame.

---

### Attack Task

Activated using external interrupt from push button.

Button press cycles through

1. Unknown ID Attack
2. Invalid RPM Attack
3. Flood Attack

---

# Node 2 (CAN IDS)

## CAN RX Interrupt

Receives every CAN frame.

Creates a CAN_Frame_t structure.

Pushes the frame into Queue1.

---

## CANReceiveTask

Priority: High

Receives CAN frames from Queue1.

Forwards every frame to Queue2.

---

## IDSDetectionTask

Priority: Above Normal

Receives CAN frames from Queue2.

Checks

- Unknown CAN ID
- Invalid RPM
- CAN Flood

If the frame is normal:

- Print vehicle information to UART.

If an intrusion is detected:

- Create IDS_Event_t
- Send to Queue3.

---

## AlertTask

Priority: Normal

Receives IDS_Event_t from Queue3.

Generates alerts by

- Printing detailed message on UART
- Turning ON Red LED
- Activating Buzzer

Returns to blocked state after handling the alert.

---

# Data Flow

```
Blue Pill
    │
    ▼
CAN Bus
    │
    ▼
STM32F407 CAN RX Interrupt
    │
    ▼
Queue1 (CAN_Frame_t)
    │
    ▼
CANReceiveTask
    │
    ▼
Queue2 (CAN_Frame_t)
    │
    ▼
IDSDetectionTask
    │
    ├──────── Normal Frame
    │              │
    │              ▼
    │         UART Logging
    │
    └──────── Attack
                   │
                   ▼
            Queue3 (IDS_Event_t)
                   │
                   ▼
              AlertTask
                   │
        ┌──────────┼──────────┐
        ▼          ▼          ▼
     UART       Red LED    Buzzer
```

---

# CAN Message IDs

| CAN ID | Description |
|---------|-------------|
| 0x100 | Engine RPM |
| 0x200 | Vehicle Speed |
| 0x300 | Temperature |

---

# Attack Modes

### Unknown ID Attack

Injects an unauthorized CAN ID.

Example:

```
ID = 0x666
```

---

### Invalid RPM Attack

Injects an RPM value outside the valid operating range.

Example:

```
RPM = 15000
```

---

### Flood Attack

Continuously transmits CAN frames with a very short interval to simulate bus flooding.

---

# Example UART Output

Normal Communication

```
ID:0x100 RPM:3000
ID:0x200 SPEED:60
ID:0x300 TEMP:35
```

Unknown ID

```
*** ALERT ***
UNKNOWN ID : 0x666
```

Invalid RPM

```
*** ALERT ***
INVALID RPM : 15000
```

Flood Attack

```
*** ALERT ***
FLOOD ATTACK
```

---

# Future Improvements

- CAN ID whitelist stored in Flash memory
- Logging detected attacks to SD Card
- CAN FD support
- Message authentication
- Machine learning based anomaly detection

