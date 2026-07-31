# 软件控制流程

## 总状态机

```mermaid
flowchart TD
    Power["上电初始化"] --> Idle["IDLE 等待"]
    Idle -->|"MODE 键"| Select["切换模式 1/2/3"]
    Select --> Idle
    Idle -->|"START 键"| Running["RUNNING 运行"]
    Running -->|"START 键"| Idle
    Running -->|"任务完成/超时/绕场一圈"| Finished["FINISHED 停止显示结果"]
    Finished -->|"START 键"| Running
    Idle -->|"CALIB 键"| Calib["MPU 零偏校准 + 舵机回中"]
    Calib --> Idle
```

## 模式一：巡线

```mermaid
flowchart TD
    A["读取 PB2-PB5"] --> B{"看到黑线?"}
    B -->|"是"| C["加权平均得到 line_error"]
    C --> D["PID 输出差速修正"]
    D --> E["左轮=基础速度+修正 右轮=基础速度-修正"]
    B -->|"否"| F["按上次误差方向低速找线"]
    E --> G{"|偏航角|>=350°?"}
    F --> G
    G -->|"是且已过起步忽略时间"| H["停车并完成"]
    G -->|"否"| A
```

跑完一圈的判定纯靠 MPU6050 累计偏航角：起步时清零，转过
`ROBOT_LINE_FINISH_YAW_DEG`（默认 350°）即视为绕场一圈完成，不再依赖
四探头全黑的视觉停车线标记。

## 模式二：滚球

```mermaid
flowchart TD
    A["K230 串口接收钢球 x_mm"] --> B{"视觉有效?"}
    B -->|"否"| C["舵机回中 PID 清零"]
    B -->|"是"| D["误差=目标位置-x_mm"]
    D --> E["滚球 PID 输出舵机脉宽增量"]
    E --> F["限制舵机最大动作幅度"]
    F --> G{"到达 +5cm 并稳定?"}
    G -->|"是"| H["目标切换为 -5cm"]
    H --> I{"到达 -5cm 并稳定?"}
    I -->|"是"| J["任务完成"]
```

## 模式三：巡线 + 滚球

模式三每个控制周期同时执行：

1. 红外巡线闭环，输出左右轮差速。
2. K230 识别钢球位置，输出舵机角度。
3. MPU6050 更新相对初始角度，用于记录和后续报告分析。

这两个闭环共享同一个 5 ms 主循环。实际调车时，先让巡线单独稳定，再逐步增加滚球 PID。
