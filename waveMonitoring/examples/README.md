# 5 分钟试跑：用 OpenFOAM 自带的 damBreak 验证本模块

这个例子不放进仓库（避免重复拷贝 OpenFOAM 的教程），直接拿 v2606 自带的 `damBreak` 做一次，
目的是确认“加载 lib + 测波 + 谱分析”这条链路在你机器上是通的。

```bash
# 0) 环境（只需要 OpenFOAM，不需要 waves2Foam）
source ~/OpenFOAM/OpenFOAM-v2606/etc/bashrc

# 1) 编译模块（只需一次）
cd <path to>/waveMonitoring && ./Allwmake

# 2) 准备算例
cp -r $FOAM_TUTORIALS/multiphase/interFoam/laminar/damBreak/damBreak /tmp/damBreakMon
cd /tmp/damBreakMon
rm -rf [1-9]* 0.[0-9]* postProcessing
cp -r 0.orig 0
```

把 `examples/waveElevation.controlDict` 里的 `waveElevation1` 块加进 `system/controlDict` 的
`functions { ... }` 里（**注意**：是加在已有 `functions` 花括号内部，不要新开一个 `functions`），
再把 `endTime` 改大一点，比如 1.0：

```bash
sed -i 's/endTime .*/endTime         1.0;/' system/controlDict
```

然后：

```bash
blockMesh
setFields        # damBreak 必须！否则全域 alpha=0，测到的高程全是 0
interFoam
```

结果：

```bash
head -12 postProcessing/waveElevation/waveElevation1/elevation.dat
```

应该看到 x=0.10 m 的测点初始 η ≈ 0.2905 m（damBreak 初始水深 0.292 m）。

再做谱分析：

```bash
waveSpectra -file postProcessing/waveElevation/waveElevation1/elevation.dat \
            -out waveSpectra -tStart 0.6
cat waveSpectra/waveStats.dat
```

> damBreak 是衰减的非周期信号，所以 `Tp` 会等于记录长度、`H1/3` 可能只有一个波；
> 这是信号本身的特点，不是程序问题。想验证谱算法，请用正弦/规则波数据
> （见 `README.md` 第 5 节的合成波校验）。
