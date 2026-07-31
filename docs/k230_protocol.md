# TI ↔ K230 第3问协议

串口为 115200、8N1，每帧 ASCII 文本，以 `\n` 结束。

## TI 发出

```text
$CMD,Q3,START,<seq>,50,-50\n
$CMD,Q3,ABORT,<seq>\n
$CMD,Q3,NEUTRAL,<seq>\n
$CMD,Q3,KEEP,<seq>\n
```

`seq` 为 1–65535 的任务序号。每次 START 前 TI 先发同序号 NEUTRAL，防止 TI
单独复位后接管 K230 的旧任务。同一个 START 序号重发时，K230 只重发 ACK，
不会把正在执行的动作重新归零。运行及 DONE 保持阶段 TI 每 100 ms 发 KEEP；
K230 450 ms 收不到同序号 KEEP 就报故障并让舵机缓回中。

## K230 返回

```text
$ACK,<seq>,START|ABORT|NEUTRAL\n
$H3,<seq>,<state>,<x_mm>,<target_mm>,<error_mm>,<servo_us>,<valid>,<elapsed_ms>\n
$DONE,<seq>,<elapsed_ms>,<error_mm>\n
$FAULT,<seq>,<code>\n
```

| state | 含义 |
|---:|---|
| 0 | 空闲 |
| 1 | 去 +5 cm |
| 2 | 去 -5 cm |
| 3 | -5 cm 稳定确认 |
| 4 | 动作完成，仍闭环保持 -5 cm |
| 5 | 故障 |

H3 每 40 ms 一帧。TI 每 200 ms 可重发 START，最多 3 次。运行阶段超过
250 ms 没有收到同序号帧即停止当前动作；DONE 后保持阶段允许 500 ms
通信抖动，超时后显示保持故障。

K230 的动作截止为 4.8 s。TI 还会检查从 K3 按下到 DONE 的本地时间和 K230
锁存的远端时间，两者都必须不超过 5.000 s；OLED 锁存并显示 TI 的本地总时间，
也就是题目要求的“按键启动到完成”。进入 DONE 后，K230 的 `elapsed_ms`
固定为动作完成值，不会继续增加。

旧版无序号 `$BALL` 调试帧已停止发送，也不作为任务心跳。只有带 `seq` 的
ACK/H3/DONE/FAULT 能维持当前任务链路。
