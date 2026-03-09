# Hardware Setup Guide

## Components

- 1x Raspberry Pi 4 (2GB+ RAM)
- 1x Electrodragon RGB Matrix Panel Drive Board v2
- 10x P4 LED Panel Modules (64x32 pixels, 256x128mm, 1/16 scan, indoor SMD RGB)
- 1x 5V power supply (40A recommended for 10 panels)
- HUB75 ribbon cables (included with panels)

## Power Requirements

Each P4 64x32 panel draws approximately 2A at full white. With 10 panels:
- Peak draw: ~20A at 5V (100W)
- Typical draw: ~8-12A at 5V (40-60W) for normal content
- **Recommended PSU**: 5V 40A (200W) to provide headroom

**WARNING**: Do not power the panels from the Raspberry Pi's 5V pins. Use a dedicated 5V PSU connected directly to the panels' power terminals.

## Panel Wiring

### Physical Layout

```
    ┌─────────┬─────────┬─────────┬─────────┬─────────┐
    │  Panel  │  Panel  │  Panel  │  Panel  │  Panel  │
    │    1    │    2    │    3    │    4    │    5    │  ← Chain 1
    │  64x32  │  64x32  │  64x32  │  64x32  │  64x32  │
    ├─────────┼─────────┼─────────┼─────────┼─────────┤
    │  Panel  │  Panel  │  Panel  │  Panel  │  Panel  │
    │    6    │    7    │    8    │    9    │   10    │  ← Chain 2
    │  64x32  │  64x32  │  64x32  │  64x32  │  64x32  │
    └─────────┴─────────┴─────────┴─────────┴─────────┘
```

### HUB75 Data Chain Connections

**Chain 1** (Electrodragon board HUB75 Output 1):
```
Board OUT1 ──► P1 IN ──► P1 OUT ──► P2 IN ──► P2 OUT ──► P3 IN ──► P3 OUT ──► P4 IN ──► P4 OUT ──► P5 IN
```

**Chain 2** (Electrodragon board HUB75 Output 2):
```
Board OUT2 ──► P6 IN ──► P6 OUT ──► P7 IN ──► P7 OUT ──► P8 IN ──► P8 OUT ──► P9 IN ──► P9 OUT ──► P10 IN
```

Each panel has a HUB75 INPUT and OUTPUT connector. Data flows from left to right. Connect the OUTPUT of one panel to the INPUT of the next panel in the chain using the included ribbon cables.

### Power Connections

Each panel has screw terminals or connectors for 5V and GND. Wire all panels in parallel to the 5V PSU:
- Connect the 5V rail to every panel's VCC/5V terminal
- Connect the GND rail to every panel's GND terminal
- Also connect the PSU GND to the Raspberry Pi GND (common ground)

```
5V PSU ──┬── Panel 1 VCC    GND PSU ──┬── Panel 1 GND
         ├── Panel 2 VCC              ├── Panel 2 GND
         ├── ...                       ├── ...
         ├── Panel 10 VCC             ├── Panel 10 GND
         └── (Pi via Electrodragon)    └── Pi GND (via Electrodragon)
```

## Electrodragon Board Setup

1. Mount the Electrodragon RGB Matrix Panel Drive Board v2 onto the Raspberry Pi GPIO header
2. The board maps directly to the Pi's GPIO pins - no additional configuration needed
3. The board supports 3 parallel HUB75 outputs - we use outputs 1 and 2
4. This board uses the **"regular"** hardware mapping in `rpi-rgb-led-matrix`

## Software Configuration

The display application is configured for this exact hardware setup:

| Parameter | Value | Description |
|-----------|-------|-------------|
| `--led-rows` | 32 | Each panel is 32 pixels tall |
| `--led-cols` | 64 | Each panel is 64 pixels wide |
| `--led-chain` | 5 | 5 panels per chain |
| `--led-parallel` | 2 | 2 parallel chains |
| `--led-multiplexing` | 0 | Standard 1/16 scan |
| `--led-hardware-mapping` | regular | Electrodragon board mapping |
| `--led-slowdown-gpio` | 2 | For Pi 4 timing |
| `--led-brightness` | 80 | Default brightness (0-100) |
| `--led-row-addr-type` | 0 | Default addressing |

## Troubleshooting

### Flickering or ghosting
- Increase `--led-slowdown-gpio` (try 3 or 4)
- Check power supply capacity
- Ensure good ground connections

### Wrong colors or garbled display
- Verify HUB75 ribbon cable orientation (pin 1 alignment)
- Check `--led-multiplexing` setting (P4 1/16 scan should be 0)
- Try different `--led-row-addr-type` values if scan pattern is wrong

### Only one row of panels works
- Verify Chain 2 is connected to Output 2 on the Electrodragon board
- Check `--led-parallel=2` is set

### Panels in wrong order
- Data flows from the board output through each panel's IN→OUT connectors
- Panel closest to the board is panel 1 (leftmost)
