% NIDS two-wheeled self-balancing robot - linearised model + PID check
% matlab/balance_model.m
%
% RECONSTRUCTION. The original MATLAB script (adapted from Appendix A of the
% IIT Delhi report and run on 20-21 Jun 2026) was not kept. This file rebuilds
% the same inverted-pendulum-on-wheels model from the parameters in the
% project report (Table 6.1) so the analysis can be re-run.
%
% What this model gives with the final gains (checked by an independent
% Python/SciPy re-implementation, Oct 2026):
%   closed-loop poles  -172.3,  -2.04 +/- 1.83j      (all stable)
%   10-90 % rise       ~17 ms   (the fast pole)
%   final value        1.078    (~8 % offset, no true integral action on theta)
%   2 % settling       ~1.0 s   (slow tail from the -2 +/- 1.8j pair)
% The original 25 Jun screenshot read as "settles in ~32 ms". That matches the
% fast rise here but not the slow tail or the offset, so either the original
% script differed or the tail was off-screen. Replace this file with the
% original if it turns up.
%
% Needs: Control System Toolbox (ss, tf, feedback, step, stepinfo, margin)

clc; clear; close all;

%% Physical parameters (project report, Table 6.1)
M_body = 1.15;        % kg, body excluding wheels and motors
M_batt = 0.432;       % kg, 3S LiPo (part of M_body; listed for reference)
M_w    = 0.106;       % kg, each wheel
R_w    = 0.05;        % m, wheel radius (100 mm wheels)
h      = 0.108;       % m, CoG height above wheel axle
I_body = 0.01306;     % kg m^2, body pitch inertia about CoG
Ke     = 0.118;       % V s/rad, back-EMF constant (= Kt)
Rm     = 5.6;         % ohm, motor terminal resistance
Jm     = 2.66e-5;     % kg m^2, rotor inertia
g      = 9.81;

%% Derived quantities
J_w     = 0.5 * M_w * R_w^2;                     % wheel inertia (uniform disc)
I_total = I_body + M_body * h^2;
M_eq    = M_body + 2*M_w + 2*(J_w + Jm)/R_w^2;
Km      = Ke / Rm;                               % torque per volt (stall)
Kb      = Ke^2 / (Rm * R_w);                     % back-EMF damping term
delta   = M_eq * I_total - (M_body * h)^2;

%% State space, states x = [p; p_dot; theta; theta_dot], input u = V_in
A = [0  1                                              0                          0;
     0  (2*Kb*M_body*h - 2*Kb*I_total/R_w)/delta       (M_body*h)^2*g/delta       0;
     0  0                                              0                          1;
     0  (2*Kb*M_eq - 2*Kb*M_body*h/R_w)/delta          M_eq*M_body*g*h/delta      0];

B = [0;
     (-2*Km*M_body*h + 2*Km*I_total/R_w)/delta;
     0;
     (-2*Km*M_eq + 2*Km*M_body*h/R_w)/delta];

C = [0 0 1 0];        % output: pitch angle theta (rad)
D = 0;

G = minreal(tf(ss(A, B, C, D)));   % cancels the pole/zero at s = 0
disp('Plant theta(s)/V(s):'); G
disp('Open-loop poles:'); disp(pole(G))      % one pole in the right half-plane

%% PID: final gains from the 25 Jun 2026 MATLAB screenshot
Kp = 450.4747;
Ki = 800.905;
Kd = 100;
Cpid = pid(Kp, Ki, Kd);

L  = minreal(Cpid * G);
CL = feedback(L, 1);

disp('Closed-loop poles:'); disp(pole(CL))

figure(1); step(CL, 1); grid on;
title('Closed-loop step response, final PID gains');

figure(2); margin(L); grid on;

S = stepinfo(CL);
fprintf('\nRise time     : %.4f s\n', S.RiseTime);
fprintf('Settling time : %.4f s\n', S.SettlingTime);
fprintf('Overshoot     : %.2f %%\n', S.Overshoot);
fprintf('Final value   : %.4f\n', dcgain(CL));

%% Interactive re-tuning (as done originally)
% sisotool(G)
