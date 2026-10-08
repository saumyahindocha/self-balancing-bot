# Bring-up and tuning

The procedure for `firmware/balance`, in order. Each step comes from a problem
that was actually hit during the build.

## 1. Electrical checks

Use the sketches in [`firmware/tests`](../firmware/tests):

1. `imu_test`: the pitch should change smoothly as you tilt the robot. At rest
   this build read about **1.5°** (18 Aug 2026). That's normal; the IMU is
   mounted at a slight angle.
2. `l298n_test`: forward, stop, reverse on both channels.
3. `motor_direction_test`: both wheels must roll the same way. On this chassis
   they didn't (18 Aug), because the motors are mirror-mounted. Fix it **once**:
   - either swap the left motor's wires on OUT3/OUT4 (the simplest and the
     recommended option), or
   - set `LEFT_MOTOR_REVERSED 1` in `balance.ino`.

   Negating the speed in `loop()` stopped the left motor instead of reversing
   it, which is why the fix lives in the motor driver function now.

## 2. Motor dead zone

The motors have no encoders and don't turn at all below a certain PWM, so small
PID corrections would vanish. Run `motor_deadzone_test` and note the first PWM
at which the wheel moves. Put that value in `DEADZONE`. The 65 in the code is a placeholder; this
build's real value hasn't been measured yet.
`applyDeadzone()` then maps commands 5-250 onto `DEADZONE`-250.

## 3. Battery

The L298N drops about 2 V, so the motors see roughly battery voltage minus 2 V.

| 3S pack | Motor sees | |
|---|---|---|
| 12.6 V (full) | ~10.6 V | excellent |
| 11.1 V (nominal) | ~9.1 V | good |
| 10.5 V | ~8.5 V | acceptable. **Charge before tuning further** |
| 9.5 V | ~7.5 V | borderline. Minimum for balancing |
| 9.0 V | ~7.0 V | too weak |

Tune between 11 and 12.6 V. A low pack is a common hidden reason tuning
"stops working".

## 4. Target angle

Hold the robot at its balance point and read the pitch. When the PWM didn't
change with pitch on 18 Aug, the suspected cause was `targetAngle` set to the
at-rest reading (about 1.5°), which makes the error zero. Start with `targetAngle = 0` and trim it later, only if the robot
drifts consistently one way.

## 5. Gains

Hold the robot upright by hand for the first attempts.

| Step | Change | Until |
|---|---|---|
| 1 | `Kp = 30, Ki = 0, Kd = 0` | Raise Kp until it actively fights back but oscillates. If it oscillates wildly, try 15-20; if it's sluggish, try 40-50 |
| 2 | Add `Kd` 0.5 → 2.0 | The oscillation is damped |
| 3 | Add `Ki` 10 → 50 | Slow drift is gone |

A check on step 1: with `Kp = 30`, a 1.5° error gives a 45 PWM command, which
`applyDeadzone()` lifts to about 95 PWM. That's enough to turn the wheels.

**Units:** this sketch works in **degrees**. The IIT Delhi gains
(Kp 441, Ki 750, Kd 0.01) and the MATLAB gains (Kp 450.47, Ki 800.91, Kd 100)
were for an error in **radians** with an output in volts or PWM. They are
starting points for the archived receiver code, not for this sketch.

## 6. Safety behaviour in the firmware

- |pitch| > 17.5°: both motors run at full power (±250) towards upright.
- |pitch| > 35°: the robot counts as fallen. Motors stop, and the integral
  and last error reset.
- The integral is clamped to ±5000 (anti-windup).

## If the motors don't move but Serial Monitor prints PWM values

1. Check for a common ground between the L298N and the Mega.
2. Check the ENA/ENB jumpers are removed.
3. Check the motors are on the right outputs.
4. Check the LiPo is connected and charged.
5. If the PWM value isn't changing, the error is zero: check `targetAngle`.
6. If it's still dead, temporarily drive ±150 straight from `loop()` (see
   `motor_direction_test`). If the wheels move, the problem is in the gains or
   the logic. If they don't, it's the wiring.

## Optional: encoders

The motors have no encoders. Dead-zone compensation is the software fix. For
speed feedback later, the options discussed were:
- an **AS5600** magnetic encoder on the output shaft (I2C, address 0x36, no
  clash with the IMU at 0x68),
- **A3144 Hall sensors** with magnets on a disc, read on interrupt pins 18/19
  (pins 2 and 3 are taken by ENA/ENB),
- a printed slotted disc with a **TCRT5000**.
