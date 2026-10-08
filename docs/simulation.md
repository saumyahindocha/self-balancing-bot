# MATLAB simulation

## Model

The robot is modelled as an inverted pendulum on wheels, following the IIT
Delhi report:
- Euler-Lagrange equations of motion for the body and the wheels
- motors modelled with inductance neglected
- linearised about upright

- **States:** wheel position *p*, wheel velocity *ṗ*, pitch *θ* and pitch rate *θ̇*
- **Input:** motor voltage *V*
- **Output:** *θ*

Parameters (project report, Table 6.1):

| Parameter | Value |
|---|---|
| Body mass (excl. wheels/motors) | 1.15 kg |
| Battery mass | 0.432 kg |
| Wheel mass / radius | 0.106 kg / 0.05 m |
| CoG height above axle | 0.108 m |
| Body pitch inertia | 0.01306 kg·m² |
| Back-EMF constant Ke = Kt | 0.118 V·s/rad |
| Motor resistance | 5.6 Ω |
| Rotor inertia | 2.66 × 10⁻⁵ kg·m² |

Computing it from these values (the reproduction in
[`matlab/balance_model.m`](../matlab/balance_model.m)) gives
θ(s)/V(s) ≈ 1.75 s / (s³ + 1.68 s² − 75.6 s − 101). The project report draft
(June 2026) gave a slightly different 1.819 s / (s³ + 1.733 s² − 74.65 s − 102.1).
It has open-loop poles at about **+8.5**, −8.9 and −1.3. The +8.5 pole is the
fall the controller has to catch, with a time constant of about 0.12 s.

## Tuning runs (20-21 Jun 2026)

The original script, based on the report's Appendix A, was run in MATLAB.
Four step-response screenshots were saved on 21 Jun, and the final gains were
recorded on 25 Jun:

| Screenshot (21 Jun) | Behaviour seen |
|---|---|
| 00:23 | Over-damped. Smooth but slow, with a steady-state offset |
| 01:18 | Unstable. The output diverged (the plot reached ~10²⁴ on the y-axis) |
| 01:35 | Stable but under-damped, oscillating for several seconds |
| 01:38 (final) | Fast rise, settling read off the plot as ~30-35 ms |

**Final gains:** Kp = 450.47, Ki = 800.91, Kd = 100.

Only the final gains are recorded. The gains behind the other three
screenshots weren't kept. The descriptions above are how the plots were read
when the project report was drafted; the screenshots themselves aren't in
this repo yet.

## Reproducing it

[`matlab/balance_model.m`](../matlab/balance_model.m) rebuilds the model from
Table 6.1 and applies the final gains. It gives:

| | Value |
|---|---|
| Closed-loop poles | −172.3, −2.04 ± 1.83j (stable) |
| 10-90 % rise time | ~17 ms |
| Final value | 1.078 (≈ 8 % offset) |
| 2 % settling time | ~1.0 s |

The fast rise agrees with the screenshot. The slow tail and the offset don't
appear in it.

**Why there's an offset despite the integral term:** θ(s)/V(s) has a zero at
s = 0. A constant voltage makes the wheels roll at a constant speed rather
than hold a constant tilt, so that zero cancels the PID's integrator. Pitch-only
PID can't remove a steady pitch offset. The usual fix is a second, outer loop
on wheel speed or position, which needs encoders (see
[tuning](tuning.md#optional-encoders)).

## Motor-driver current (21 Jun 2026)

The simulated controller often slams the output from full forward to full
reverse. In hardware, every direction reversal causes a current spike through
the L298N, so the concern raised during simulation was a real one.

Mitigations noted:
- slew-rate limiting the PWM command in code (e.g. ±30 per loop)
- a heatsink on the L298N
- 100 nF capacitors across each motor
- a bulk capacitor at the L298N input
- a lower Ki
- polyfuses
- a BTS7960 driver if the stall current is above about 2 A

The slew limiter isn't in the current firmware yet.

## Gains in the firmware

The MATLAB gains act on θ in radians and output volts. Mapping them onto the
Arduino code needs a scale factor for radians, volts and PWM. The archived
receiver uses Kp 450 / Ki 750 / Kd 0.1 in radians. `firmware/balance` works in
degrees and is tuned by hand from Kp = 30 (see [tuning](tuning.md)).
