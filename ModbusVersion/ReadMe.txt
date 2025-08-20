Bring‑Up Checklist

Networking: Set PC to 192.168.1.x/24, ping 192.168.1.50.

Firmware: Flash the PMC sketch; confirm it answers on TCP 502.

Node‑RED: Import the flow; install required palettes; deploy.

Heartbeat: Confirm HB_ECHO follows the sequence; ST_HEARTBEAT_OK=1.

Scale: Implement ADS1234 read + calibration; verify MASS_G_X10 matches a test weight.

Actuators: Test solenoid output (air water off), test 0–10 V span into a voltmeter.

Flow calc: With water running, confirm FLOW_GPS is stable and sign/direction is correct.

PID: Start with tiny gains (e.g., Kp=0.5, Ki=0.1, Kd=0), increase slowly.

Safety: Pull E‑Stop → verify FAULT 101; comms unplug → FAULT 105 handling as desired.

Docs: Print the register map for the control cabinet door.

Notes & Tuning Tips

Derivative window: For noisy mass signals, increase SP_DERIV_WINDOW and/or low‑pass mass before differentiating.

Hysteresis: Use SP_FLOW_HYST to prevent valve hunting around the target.

Valve limits: Keep a conservative SP_VALVE_MIN/MAX until you know the plant response.

Logging: You can add a CSV logger in Node‑RED (append rows at SP_LOG_INTERVAL).

Scaling changes: If you change scaling, bump a map version number (add another IR with VERSION) so the HMI can adapt.

Awesome — I’ve dropped a complete starter kit into the canvas on the right:

a tailored Modbus TCP register map (signals + setpoints),

a Portenta firmware skeleton with the IDLE/FILL/BALANCE_TEST/TEST/SHUTDOWN state machine, heartbeat, and safe write-handling,

an importable Node-RED Dashboard flow wired to that map,

plus a short bring-up checklist and tuning notes.

If you want, I can also:

add a CSV data logger and alarm banner to the Node-RED flow,

stub in a real ADS1234 driver and a basic two-point calibration routine,

tweak the register map to match your exact wiring (pin numbers, scaling, etc.).

Tell me what you’d like adjusted, and I’ll update the kit.