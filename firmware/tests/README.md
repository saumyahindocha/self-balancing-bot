# Bench tests

Small standalone sketches for checking one subsystem at a time before running
the balance code. Upload one, open Serial Monitor at **115200 baud**, and check
for the result described below.

| Sketch | Board | Checks | Pass looks like |
|---|---|---|---|
| `imu_test` | Mega | MPU6500 on I2C (SDA 20, SCL 21) | "IMU OK", then pitch moves smoothly between about -90 and +90 as you tilt it |
| `l298n_test` | Mega | L298N control lines, both channels | Forward / stop / reverse every few seconds. With no motors attached, a multimeter on OUT1-OUT2 and OUT3-OUT4 reads about +V, 0, -V |
| `motor_deadzone_test` | Mega | Lowest PWM that turns a motor | Note the PWM printed when the right motor first moves, then put it in `DEADZONE` |
| `motor_direction_test` | Mega | Both wheels turn the same way for the same command | Both wheels roll forward together, then backward together |
| `ir_test` | Mega | 3 IR sensors on pins 30/32/34 | Slide over black tape on white paper: STRAIGHT / LEFT / RIGHT / SHARP ... |
| `nrf_tx_test` + `nrf_rx_test` | Nano + Mega | NRF24L01 link | Nano prints `Sent: OK`, Mega prints the changing X/Y values |

**Lay the robot on its side, or let the wheels hang off a table edge, for any
motor test.** Nothing is balancing it while these run.

If the motors don't move but Serial Monitor shows PWM values, check these in
order:
1. A wire joins L298N GND and Mega GND.
2. The ENA/ENB jumpers are removed and wired to pins 2 and 3.
3. The motor is on the output this sketch drives (OUT1/OUT2 = IN1/IN2).
4. The LiPo is connected to the L298N 12 V terminal. USB alone does not power the motors.
