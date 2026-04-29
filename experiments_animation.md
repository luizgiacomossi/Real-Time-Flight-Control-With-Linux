# Flight Experiments — Animations

## SITL Example: 4-Waypoint Sequence

This animation shows the Software-in-the-Loop (SITL) simulator tracking a 4-waypoint trajectory under nominal conditions. It illustrates the closed-loop controller driving the drone through sequential position setpoints, with the active waypoint highlighted in real time.

**Waypoints:** `(0,0,3)` → `(3,0,3)` → `(3,3,5)` → `(0,0,1)`

![4-Waypoint Sequence](animations/anim_waypoints.gif)

---

## Paper Experiments: Best vs. Worst Case Under CPU Stress

The following two animations are generated directly from the SITL experiment data collected on the Raspberry Pi. Both runs use the same 4-waypoint trajectory and the same CPU stress load (`stress-ng`). The only difference is the Linux scheduling policy.

### Best Case — `PREEMPT_RT` kernel · `SCHED_FIFO` priority 99 · Under Stress

The PREEMPT_RT kernel guarantees bounded wake-up latency even under heavy CPU load. The controller receives CPU time within its 4 ms deadline on every iteration, resulting in stable, accurate trajectory tracking.

![Best Case: PREEMPT_RT FIFO p99 under stress](animations/anim_best_preempt_rt_fifo_p99_stress.gif)

---

### Worst Case — Standard kernel · `SCHED_OTHER` nice 0 · Under Stress

The standard Linux kernel does not offer real-time guarantees. Under `stress-ng` load, the scheduler delays the control thread by tens of milliseconds. Because the drone continues to fly with stale motor commands during each delay, the tracking error accumulates and the trajectory diverges.

![Worst Case: Standard kernel OTHER n0 under stress](animations/anim_worst_standard_other_n0_stress.gif)
