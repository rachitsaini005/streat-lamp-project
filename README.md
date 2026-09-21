# Environmental Light Intensity Dependent Street Lamp

A street lamp that adjusts its own brightness to the ambient light around it. A light dependent resistor (LDR) senses the light level, an ESP32 microcontroller reads it, and the lamp's LEDs are brightened or dimmed accordingly, so energy is used only when the light is actually needed.

Minor project for **Analog Circuits (UES301)**, Electrical Engineering, Thapar Institute of Engineering & Technology (TIET), Patiala, July–December 2024.

**Team:** Aashray Sharma, Parth Gupta, Tanav Pathak
**Supervisor:** Dr. Sangeeta Kamboj, Assistant Professor (EIED)

## How it works

![Block diagram](docs/block_diagram.png)

1. Ambient light falls on the LDR, whose resistance decreases as light increases.
2. The ESP32 reads the resulting signal and decides the LED brightness.
3. As ambient light falls the LEDs are brightened; as natural light rises they are dimmed or switched off.
4. A DC supply powers both the ESP32 and the LEDs. The ESP32's Wi-Fi leaves room for later IoT / smart-city integration.

## Hardware

| Component | Role |
|---|---|
| Light dependent resistor (LDR) | Senses ambient light |
| ESP32 microcontroller | Reads the sensor and controls LED brightness |
| LEDs (red, green, yellow) | Light sources of the lamp |
| Resistors | LED current limiting and circuit operating levels |
| DC power supply | Powers the ESP32 and LEDs (USB in the diagram) |
| Breadboard and jumper wires | Connections |

## Circuit diagram

![Circuit diagram](docs/circuit_diagram.png)

## Repository contents

```
.
├── README.md
├── report/
│   ├── Street_Lamp_Project_Report.docx   # full project report (editable)
│   └── Street_Lamp_Project_Report.pdf    # same report as PDF
└── docs/
    ├── circuit_diagram.png               # breadboard wiring diagram
    └── block_diagram.png                 # system block diagram
```

## Status

- Project report and diagrams: included.
- Firmware (ESP32 code): not yet added to this repository.
