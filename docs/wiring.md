# Wiring

Pin numbers match the firmware. **Bold** rows are used by the current
balance-only build. The rest belong to the shelved remote and line-following
features.

## Robot (Arduino Mega 2560)

### Power

| From | To | Wire |
|---|---|---|
| 3S LiPo + | L298N `12V` terminal | 18 AWG red |
| 3S LiPo − | L298N `GND` terminal | 18 AWG black |
| L298N `GND` | Mega `GND` (**common ground, mandatory**) | 22 AWG black |
| Laptop USB | Mega USB-B (logic power) | USB cable |

Do not power the Mega from the L298N `5V` output. Motor switching noise on it
corrupts the IMU readings.

### L298N motor driver

| L298N | Mega | Notes |
|---|---|---|
| **ENA** | **2** (PWM) | Right motor speed. **Remove the ENA jumper.** |
| **IN1** | **12** | Right motor direction |
| **IN2** | **11** | Right motor direction |
| **ENB** | **3** (PWM) | Left motor speed. **Remove the ENB jumper.** |
| **IN3** | **6** | Left motor direction |
| **IN4** | **7** | Left motor direction |
| **OUT1 / OUT2** | Right motor | 18 AWG |
| **OUT3 / OUT4** | Left motor | 18 AWG. Swap these two if the left wheel turns backwards (see [tuning](tuning.md)) |

With the ENA/ENB jumpers left on, the enable pins are tied to 5 V, so
`analogWrite()` has no effect. On 9 Aug 2026 a photo of the board showed the
jumpers still fitted, and OUT3/OUT4 appeared empty.

### MPU6500 IMU

| MPU6500 | Mega |
|---|---|
| **VCC** | **3.3V** |
| **GND** | **GND** |
| **SDA** | **20** |
| **SCL** | **21** |

Mount it rigidly, as close to the chassis centre as possible. Any play adds
noise to the pitch reading.

### NRF24L01 receiver (shelved)

| NRF24L01 | Mega |
|---|---|
| VCC | 3.3V, with a 100 µF electrolytic across VCC/GND, legs as short as possible, at the module |
| GND | GND |
| CE | 48 |
| CSN | 49 |
| SCK | 52 (hardware SPI) |
| MOSI | 51 (hardware SPI) |
| MISO | 50 (hardware SPI) |
| IRQ | not connected |

### IR line sensors (shelved)

| Sensor | OUT | VCC / GND |
|---|---|---|
| Left | 30 | 5V / GND |
| Middle | 32 | 5V / GND |
| Right | 34 | 5V / GND |

Mount them on the bottom plate, 30 mm apart and about 35 mm above the ground.
The IIT Delhi code used pins 49/51/53, but 51 and 53 are the Mega's SPI MOSI
and SS pins, so the sensors were moved to free pins to make room for the NRF
module.

### Mega pin summary

| Pin | Function |
|---|---|
| 2 | Right motor PWM (ENA) |
| 3 | Left motor PWM (ENB) |
| 6, 7 | Left motor IN3, IN4 |
| 11, 12 | Right motor IN2, IN1 |
| 20, 21 | IMU SDA, SCL |
| 30, 32, 34 | IR left, middle, right |
| 48, 49 | NRF CE, CSN |
| 50, 51, 52 | NRF MISO, MOSI, SCK |

## Remote (Arduino Nano), shelved

| Part | Pin | Nano |
|---|---|---|
| NRF24L01 | VCC | 3V3, with 100 nF + 100 µF at the module |
| | GND | GND |
| | CE / CSN | D9 / D10 |
| | SCK / MOSI / MISO | D13 / D11 / D12 |
| KY-023 joystick | VCC / GND | 5V / GND |
| | VRX / VRY | A0 / A1 |
| | SW | D2 |
| Mode toggle switch | — | D2 to GND (parallel with joystick SW) |
| 9 V battery | + / − | VIN / GND (a power toggle switch was in the parts list) |

## Protection parts (recommended, add when convenient)

These came out of the 21 Jun MATLAB current-spike question and the 8 Aug
motor-noise question:
- **Across each motor's terminals:** a 100 nF ceramic capacitor (50 V),
  soldered directly on the motor tabs.
- **Optional:** a 10 nF ceramic from each motor terminal to the motor case.
- **At the L298N 12V/GND input:** a 100 µF electrolytic (25 V+). A larger
  bulk capacitor (~1000 µF) was also suggested.
- **On the L298N:** a finned heatsink. Cheap modules realistically handle only
  about 1-1.5 A per channel.
- **If the motors' stall current is above about 2 A:** consider a BTS7960
  driver instead of the L298N.
