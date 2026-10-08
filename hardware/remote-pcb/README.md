# Remote controller PCB

A small board for the handheld remote: an Arduino Nano reads a KY-023
joystick and sends it to the robot over an NRF24L01+PA+LNA radio.

- **Designed in:** EasyEDA (schematic and layout), May 2026, by Saumya Hindocha
- **Enclosure:** pistol-grip handheld shell designed in Fusion 360, 3D-printed in June 2026
- **Status:** the mentor verified the design (June 2026), but it was never
  sent for fabrication. On 21 Jun 2026 it was decided to build the remote on
  **perfboard** instead, with the same circuit and pins. The perfboard was in
  hand by 24 Jun. The whole remote was shelved with
  the NRF link on 18 Aug 2026.

## Circuit

| Ref | Part | Connection |
|---|---|---|
| U1 | Arduino Nano | — |
| U2 | NRF24L01 + PA + LNA (external antenna) | CE D9, CSN D10, SCK D13, MOSI D11, MISO D12, VCC 3V3, GND. IRQ not connected |
| C | 100 nF decoupling at NRF VCC/GND | A 100 µF electrolytic in parallel was recommended for the perfboard build |
| H1 | 5-pin joystick header (KY-023) | 1 GND, 2 VRY to A1, 3 VRX to A0, 4 +5 V, 5 SW to D2 |
| H2 | 2-pin power input | 9 V battery to Nano VIN |
| — | Voltage regulator (B35281411 as read from the screenshot) | Power section |
| SW | Mode toggle switch (perfboard build) | Between D2 and GND, in parallel with the joystick button |

CE = D9 and CSN = D10 were confirmed against the schematic on 2 Jun 2026. The
joystick pins were read from a screenshot and confirmed the same day ("the
joystick pattern is perfect").

## Files not in this repo yet

The EasyEDA project, the schematic and layout exports, and the Gerbers. Only
screenshots of them were shared, and those weren't kept. To complete this
folder, export from EasyEDA:
- `remote.json` (File > Save As / Export EasyEDA source)
- `schematic.pdf` / `schematic.svg`
- `gerbers.zip` and `netlist.net`

Add the Fusion 360 enclosure as `../enclosure/` (`.f3d` and `.stl`).
