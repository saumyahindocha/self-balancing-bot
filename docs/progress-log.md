# Progress log

Dates are IST. Most work sessions ran late at night, so a session that started
on the evening of the 1st shows up here under the 2nd.

---

### 24 May 2026: Kick-off and research
- Started from the IIT Delhi summer-internship report on a two-wheeled
  self-balancing, path-following robot. Its stack: Arduino Mega 2560, MPU6500,
  L298N, 3 IR sensors, 12 V geared motors, a three-tier acrylic chassis, a
  Kalman filter, and PID with Kp ≈ 441, Ki ≈ 750, Kd ≈ 0.01.
- Budget check: the build looked achievable for under ₹10,000 at 12 V. The
  battery and motors are the main costs.
- Studied PID basics and why cascaded PID beats LQR/MPC on a small
  microcontroller.

### 27 May 2026: Remote design and plan
- Added a **wireless manual override** to the IIT Delhi design. A handheld
  remote with an Arduino Nano, a KY-023 joystick, an NRF24L01+PA+LNA radio and
  a 9 V battery, in a pistol-grip enclosure designed in Fusion 360.
- Designed the remote's own PCB in EasyEDA, with capacitors at the NRF supply.
- Decisions:
  - the joystick only steers, and the PID keeps balancing underneath
  - the NRF receiver connects straight to the Mega
  - polling instead of interrupts (a 50 Hz loop is fast enough for joystick input)
- Six-week plan, in order: get it standing, tune the PID, then add IR, then
  the NRF link.

### 2 Jun 2026: Pin map, first firmware, library swap
- PCB waiting for the mentor's review. Planned a component-shopping trip
  (Lamington Road suggested) with a full BOM and wire list: 18 AWG for power,
  22 AWG for signals.
- Read the remote schematic and confirmed **CE = D9, CSN = D10**. The joystick
  is on A0/A1/D2.
- Found a **pin clash**. The IIT Delhi code put the IR sensors on Mega pins 49,
  51 and 53, but 51 and 53 are SPI pins the NRF needs. The IR sensors moved to
  **30/32/34**, with the NRF on CE 48 / CSN 49.
- Wrote the first transmitter and receiver code (PID + Kalman + IR + NRF).
- The Bolder Flight `mpu6500` library wasn't in the Library Manager, so the
  IMU code was ported to **FastIMU**. That fixed three compile errors:
  `IMU_ADDRESS` became `0x68`, `calibrateAccel/Gyro` became
  `calibrateAccelGyro`, and a missing closing brace. The hardware and wiring
  didn't change.

### 20-21 Jun 2026: Firmware v2, MATLAB, perfboard
- All parts were in hand except the acrylic plates, the PCB and the motors.
  The mentor had verified the PCB design, but Saumya still had doubts about it.
- The report doesn't give an RPM for its final 12 V motor; the 100-150 RPM
  target was a general recommendation. Rule of thumb from here on: favour
  torque (aim for at least 3-5 kg·cm) over RPM.
- Rewrote both sketches and **compiled cleanly**:
  - checks `radio.begin()` on startup
  - debounces the mode button without blocking (the old `delay(300)` froze the
    balance loop)
  - clamps the integral (anti-windup) and guards against `dt = 0`
  - drops the unused roll filter
  - fixes channel 76, starts at `PA_LOW`, and closes a gap in the tilt-threshold
    boundaries
- The PCB couldn't be ordered. Chose **perfboard** over soldering wires
  directly: the remote goes on perfboard, the robot gets a perfboard breakout
  for its module connections, and every module sits in female headers.
- **MATLAB:** ran the linearised model and tuned the PID. Four step-response
  screenshots were saved between 00:23 and 01:38 on 21 Jun: over-damped,
  unstable, under-damped, then the final tuned response (see
  [simulation](simulation.md)). Raised a real concern: hard direction reversals cause current
  spikes in the L298N. Mitigations: slew-rate limit, heatsink, motor
  capacitors, a bulk capacitor, lower Ki, and a stronger driver if needed.
- Motors due on **30 Jun**. In the meantime the IMU, IR sensors, NRF link and
  L298N were to be checked without motors.

### 22 Jun 2026: Chassis files
- Spotted what looked like an ESP module in one of the report's photos. The
  report text doesn't mention one, and that question was left open. This
  design doesn't need one either way: the Mega handles the robot, and the
  single Nano lives in the remote.
- Generated laser-cut SVGs for the three 230 x 100 x 6 mm plates, with
  Ø6.5 mm rod holes and IR sensor holes (see
  [hardware/chassis](../hardware/chassis)). Flagged that the report's plate
  sizes disagree with each other.

### 24-25 Jun 2026: Fabrication and bench tests
- **Acrylic plates laser-cut.** Perfboard for the transmitter ready; remote
  enclosure 3D-printing.
- Wrote isolated bench-test sketches: NRF link, IMU, IR, and an L298N check
  with a multimeter (see [firmware/tests](../firmware/tests)).
- Drew wiring schematics for the remote (including the **IR/manual toggle
  switch on D2**) and the robot.
- Final MATLAB gains recorded: **Kp = 450.47, Ki = 800.91, Kd = 100.**
- Shared progress photos: Fusion 360 models of the robot and remote, a sketch,
  the built transmitter, and the MATLAB plots.

### 26-30 Jun 2026: Project report
- Started generating a full project report (Word) from the IIT Delhi theory
  plus this build's work.

### 28 Jul 2026: ITSP documentation
- Filled in the ITSP project form: team NIDS, mechatronics domain, team members
  and mentors, problem statement, proposed solution, work done, timeline and
  expected outcome.

### 8 Aug 2026: No encoders, dead zone, noise
- The planetary motors have **no encoders**. Fix in software first:
  - `applyDeadzone()` maps small PID outputs above the motors' starting PWM
  - a dead-zone finder sketch measures that PWM
  - suggested lowering `low_threshold` to 0.8° in the full receiver code
    (not applied; the archived code still uses 1.2°)
- Encoder options kept for later: AS5600, Hall sensors, or an optical disc.
- Motor noise suppression: a 100 nF capacitor across each motor (plus optional
  10 nF to the case) and 100 µF at the L298N input.

### 9 Aug 2026: First motor bring-up
- The dead-zone sketch printed PWM values but **the motors didn't turn.**
  Common ground and wiring were rechecked. From a photo of the board, two
  likely faults were identified (not confirmed afterwards):
  1. the **ENA/ENB jumpers still fitted**, which overrides the PWM
  2. OUT3/OUT4 **appeared empty**, so only one motor looked wired

### 18 Aug 2026: Scope cut, balance only
- **Shelved the NRF remote and IR line following** because they needed too
  much debugging. New goal: just balance, same pins, gains easy to edit.
- New `firmware/balance` sketch with Kalman pitch, PID, anti-windup,
  dead-zone mapping, a 17.5° recovery zone and a 35° stop.
- Debugging:
  - IMU working; pitch at rest about **1.5°**
  - PWM wasn't changing as the pitch changed. Suspected cause: `targetAngle`
    had been set to the rest reading, making the error zero. Reset it to 0 and
    dropped to Kp = 30, Ki = 0, Kd = 0.
  - the motors **still didn't respond to the PID's PWM**. That was never
    explained and is still open.
  - a direct ±150 motor test (bypassing the PID) spun both motors, but **in
    opposite directions** (mirror-mounted). Negating the left motor's speed made it **stop** instead
    of reverse.
  - fix: swap the left motor's wires on OUT3/OUT4, or flip IN3/IN4 inside
    `setMotorSpeed()`
- Minimum battery voltage for balancing: **about 9.5 V** (the L298N drops
  about 2 V). Tune between 11 and 12.6 V and charge below 10.5 V.

### 5 Sep 2026: BOM
- Re-issued the component list. Shelving the remote takes the
  NRF/Nano/PCB lines out of the active build.

### 8 Oct 2026: Moved to GitHub
- Consolidated the project into this repository. While doing so:
  - **fixed the FastIMU calibration call.** `calibrateAccelGyro(200)` passed
    200 as a `calData*` pointer, and the Arduino IDE's `-fpermissive` turned
    that error into a warning, so the IMU biases were never applied. It now
    uses `calibrateAccelGyro(&calib)` followed by `init(calib)`.
  - added a `LEFT_MOTOR_REVERSED` flag so the motor-direction fix lives in one
    place
  - fixed the archived receiver's left/right swap in `motorSpeedCtrl()`
  - checked every sketch against the real FastIMU and RF24 headers
  - rebuilt the MATLAB model from the report parameters. The model is stable
    at the final gains with a fast ~17 ms rise, but it has an ~8 % offset and
    a ~1 s tail (see [simulation](simulation.md)).

**Open:** the chat ends on 18 Aug 2026. At that point the IMU readings and the
direct motor test worked, but the motors weren't yet responding under PID
control, and the direction fix had been proposed but not confirmed. Chassis
assembly and any balancing result aren't recorded.
