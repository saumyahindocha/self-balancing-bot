# NIDS: Two-Wheeled Self-Balancing Robot

A two-wheeled inverted-pendulum robot built on an Arduino Mega. An MPU6500
IMU feeds a Kalman filter for pitch, and a PID loop drives two 12 V planetary
gear motors through an L298N to keep the robot upright. The first design also
had IR line following and a handheld NRF24L01 remote with an IR/manual mode
switch. Those parts are built and their code is archived here, but they were
set aside to get the balancing working first.

Built by **Team NIDS** at IIT Bombay for the Institute Technical Summer
Project (ITSP) 2026, mechatronics domain, May-Aug 2026.

<p align="center">
  <img src="docs/images/balancing.gif" alt="The robot balancing on its own on a tiled floor" width="260">
</p>

> **Status:** the robot balances on its own. The
> [full video](docs/media/balancing-unassisted.mp4) shows it standing
> unassisted for about 50 seconds before a nudge. It was exhibited at the ITSP
> 2026 showcase. The PID gains used for that run weren't recorded, so
> `firmware/balance` still has the starting values from the tuning guide. See
> the [progress log](docs/progress-log.md).

## How it works

```mermaid
flowchart LR
    IMU["MPU6500<br/>accel + gyro"] -- I2C --> KF
    subgraph MEGA ["Arduino Mega 2560 (USB powered)"]
        KF["Kalman filter<br/>pitch θ"] --> PID["PID<br/>e = θ_target − θ"] --> DZ["dead-zone<br/>compensation"]
    end
    DZ -- "PWM + direction" --> DRV["L298N"]
    BAT["3S LiPo 11.1 V"] --> DRV
    DRV --> M["2 × 12 V planetary<br/>gear motors"]
    M -. "robot tilts" .-> IMU
    IR["3 × IR sensors<br/>(shelved)"] -.-> MEGA
    RF["NRF24L01 ← remote<br/>(shelved)"] -.-> MEGA
```

1. **Sense.** FastIMU reads the MPU6500 over I2C at 400 kHz. Pitch is
   computed from the accelerometer (`atan2`) and fused with the gyro rate in a
   two-state Kalman filter (angle and gyro bias).
2. **Decide.** PID on the pitch error, with the integral clamped to ±5000.
   Beyond ±17.5° the robot drives at full power back towards upright, and
   beyond ±35° it counts as fallen and stops.
3. **Act.** The motors have no encoders and don't move below a certain PWM,
   so commands are remapped above that threshold. You measure the threshold
   with a bench sketch. Both wheels get
   the same command, since the robot only balances and doesn't steer.

The control design follows a linearised Euler-Lagrange model. It was tuned in
MATLAB to Kp = 450.47, Ki = 800.91, Kd = 100, and those gains are reproduced
and examined in [docs/simulation.md](docs/simulation.md).

## Hardware

| | |
|---|---|
| Controller | Arduino Mega 2560 |
| IMU | MPU6500 (I2C, 0x68) |
| Motor driver | L298N dual H-bridge |
| Motors | 2 × 12 V DC planetary gear motor, no encoder |
| Wheels | 100 mm × 44 mm |
| Power | 3S LiPo 11.1 V for the motors; USB for the logic |
| Chassis | 3 × 230 × 100 × 6 mm laser-cut acrylic plates on M6 rods |
| Remote (shelved) | Arduino Nano, KY-023 joystick, NRF24L01+PA+LNA, 9 V; custom EasyEDA PCB ([Gerbers](hardware/remote-pcb/gerbers)), built on perfboard |

Full part list with cost estimates: [hardware/bom.csv](hardware/bom.csv).
The parts in use add up to roughly ₹5.6-9.2k.

### Pinout (Mega)

| Function | Pin |
|---|---|
| Right motor ENA / IN1 / IN2 | 2 / 12 / 11 |
| Left motor ENB / IN3 / IN4 | 3 / 6 / 7 |
| IMU SDA / SCL | 20 / 21 |
| *IR left / mid / right* | *30 / 32 / 34* |
| *NRF CE / CSN / MISO / MOSI / SCK* | *48 / 49 / 50 / 51 / 52* |

Full wiring, power rules and protection parts: [docs/wiring.md](docs/wiring.md).

## Gallery

| | |
|---|---|
| ![Robot CAD in Fusion 360](docs/images/cad-robot-fusion360.jpg) | ![Assembled robot, 18 Aug 2026](docs/images/assembled-robot-2026-08-18.jpg) |
| Robot CAD (Fusion 360) | Assembled robot during PID bring-up, 18 Aug 2026 |
| ![Remote CAD in Fusion 360](docs/images/cad-remote-fusion360.jpg) | ![3D-printed remote enclosure](docs/images/remote-enclosure-print.jpg) |
| Remote enclosure CAD: KY-023, PCB, 9 V holder | 3D-printed pistol-grip enclosure |
| ![Bench setup](docs/images/bench-mega-nano-nrf.jpg) | ![ITSP 2026 exhibit](docs/images/itsp-2026-exhibit.jpg) |
| Bench testing: Mega, Nano and both NRF24L01 modules | At the ITSP 2026 exhibition |

![Remote PCB layers](docs/images/remote-pcb-layers.png)
*Remote PCB, rendered from the Gerbers (EasyEDA Pro, 26 May 2026).*

Also included: the [exhibition poster](docs/images/itsp-2026-poster.jpg) and
a [handheld motor-response test](docs/media/motor-response-handheld.mp4).

## Repository

```
firmware/
  balance/               current build: balance-only controller
  tests/                 bench sketches: IMU, IR, L298N, dead zone, direction, NRF link
  archive/
    receiver_nrf_ir/     full robot code: balance + IR line following + NRF manual override
    transmitter/         remote: joystick -> NRF24L01
hardware/
  bom.csv                bill of materials
  chassis/               laser-cut plate SVGs (1:1, mm)
  remote-pcb/            remote PCB notes, pin map and Gerbers
matlab/
  balance_model.m        linearised model + PID step-response check
docs/
  progress-log.md        dated build log
  tuning.md              bring-up and PID tuning procedure
  wiring.md              wiring tables
  simulation.md          MATLAB model, tuning runs and findings
  images/, media/        photos, CAD screenshots, PCB render, videos
```

## Getting started

1. Install the Arduino IDE and these libraries from the Library Manager:
   - **FastIMU** (LiquidCGS)
   - **RF24** (TMRh20), needed only for the archived and NRF test sketches
2. Wire the robot as in [docs/wiring.md](docs/wiring.md). **Remove the L298N
   ENA/ENB jumpers** and tie the L298N GND to the Mega GND.
3. Run the bench tests in [firmware/tests](firmware/tests): `imu_test`, then
   `motor_direction_test`, then `motor_deadzone_test`.
4. Open `firmware/balance/balance.ino` and set `DEADZONE` and
   `LEFT_MOTOR_REVERSED`. Upload at 115200 baud. Hold the robot upright and
   still for the ~4 s calibration.
5. Tune following [docs/tuning.md](docs/tuning.md), starting at Kp = 30,
   Ki = 0, Kd = 0, with the pack above 10.5 V.

## What's next

- [x] Balance unassisted (ITSP 2026 showcase)
- [ ] Record the final tuned gains and dead-zone value in `firmware/balance`
- [ ] Add a PWM slew-rate limiter and the motor noise capacitors
- [ ] Wheel encoders (AS5600 or Hall sensors), then an outer speed/position loop to remove drift
- [ ] Bring back the IR line following, then the NRF remote, on top of a stable balance loop
- [x] Gerbers and build photos
- [ ] Add the EasyEDA source project and the Fusion 360 files

## Team

**Team NIDS:** Saumya Hindocha (team lead), Nimay Shah, Divit Ghorpade, Ira Krishnan.
Mentors: Avaneesh Khadye and Ayush Lokandi.

## Acknowledgements

The mechanical layout, dynamic model, Kalman filter and IR rule table are
based on an IIT Delhi summer-internship report on a two-wheeled
path-following self-balancing robot (R. Kishore, supervised by Prof.
Subashish Datta). The motor and IMU pins also come from the report. The
IR pins were moved and the NRF pins added by this team, and so were the
wireless override, the firmware rewrite and everything after that.
