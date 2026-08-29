# OpenFOAM 在 VS Code 中的代码识别设置

本文记录如何让 VS Code 正确识别 OpenFOAM 的头文件、预处理宏和你自己写的 OpenFOAM 代码，使编写/编译 OpenFOAM 程序时拥有完整的智能提示、跳转定义和报错定位。

## 为什么需要设置

默认情况下 VS Code 存在两个问题：

1. **不识别 OpenFOAM 的文件后缀**。OpenFOAM 的源文件用大写 `.C`、头文件用大写 `.H`，而 VS Code 内置语言表只登记了小写 `.c` / `.h`（Linux 文件后缀区分大小写），因此 `.C` / `.H` 文件会被当成纯文本，没有代码补全和语法高亮。
2. **找不到 OpenFOAM 头文件**。OpenFOAM 通过 `wmake` 把每个模块的头文件软链接到 `src/<module>/lnInclude/`，编译时用 `-I$(FOAM_SRC)/<module>/lnInclude` 引用；VS Code 的 C/C++ 扩展（`ms-vscode.cpptools`）不知道这些目录，就会报 `#include errors detected`，并且整个文件的语法分析和提示都会失效。

## 前置条件

- VS Code 已安装 **C/C++** 扩展（`ms-vscode.cpptools`）。
- OpenFOAM 已经成功编译过，`src` 下存在 `lnInclude` 目录（例如 `$FOAM_SRC/OpenFOAM/lnInclude`）。
- 知道自己的 OpenFOAM 安装路径和编译选项，例如：
  ```bash
  echo $WM_PROJECT_DIR      # 例如 /home/<user>/OpenFOAM/OpenFOAM-v2206
  echo $WM_OPTIONS          # 例如 linux64GccDPInt32Opt
  echo $FOAM_SRC            # 例如 /home/<user>/OpenFOAM/OpenFOAM-v2206/src
  ```

## 方案一：全局用户设置（推荐，所有工程生效）

在 VS Code 用户设置文件（Linux：`~/.config/Code/User/settings.json`；Windows：`%APPDATA%\Code\User\settings.json`）中配置以下内容。路径请替换成你自己机器的实际路径。

### 1. 文件关联：让 `.C` / `.H` 作为 C++ 解析

```jsonc
"files.associations": {
    "*.C": "cpp",
    "*.H": "cpp"
}
```

### 2. C/C++ 全局默认配置

```jsonc
"C_Cpp.default.compilerPath": "/usr/bin/g++",
"C_Cpp.default.cppStandard": "c++14",
"C_Cpp.default.cStandard": "c11",
"C_Cpp.default.intelliSenseMode": "linux-gcc-x64",
"C_Cpp.default.defines": [
    "WM_DP",
    "WM_LABEL_SIZE=32"
],
"C_Cpp.default.includePath": [
    "${workspaceFolder}/**",
    "/home/<user>/OpenFOAM/OpenFOAM-v2206/src/OpenFOAM/lnInclude",
    "/home/<user>/OpenFOAM/OpenFOAM-v2206/src/finiteVolume/lnInclude",
    "/home/<user>/OpenFOAM/OpenFOAM-v2206/src/meshTools/lnInclude",
    "/home/<user>/OpenFOAM/OpenFOAM-v2206/src/OSspecific/POSIX/lnInclude"
    // ... 其余 src 下所有模块的 lnInclude 目录，见下文自动生成命令
],
"C_Cpp.default.browse.path": [
    "${workspaceFolder}",
    "/home/<user>/OpenFOAM/OpenFOAM-v2206/src",
    "/home/<user>/OpenFOAM/OpenFOAM-v2206/applications"
],
"C_Cpp.default.browse.limitSymbolsToIncludedHeaders": true
```

说明：

- `defines` 必须与 `$WM_OPTIONS` 一致。`linux64GccDPInt32Opt` 对应 `WM_DP` + `WM_LABEL_SIZE=32`；如果是单精度（SP）或 64 位标签（Int64），请相应改为 `WM_SP` / `WM_LABEL_SIZE=64`。这两个宏来自 `wmake/rules/General/general` 的编译参数。
- `includePath` 需要**逐个列出** OpenFOAM 的所有 `lnInclude` 目录，不要写成 `/.../src/**/lnInclude` 这种“中间带 `**`”的形式——C/C++ 扩展不支持路径中间的 globstar（见 [microsoft/vscode-cpptools#12070](https://github.com/microsoft/vscode-cpptools/issues/12070)），`**` 只支持放在路径末尾做递归搜索。
- `browse.path` 让扩展为**跨文件**符号建立索引，这样其它文件里定义的类/函数也能被识别和跳转。
- 如果代码中用到 ThirdParty 的 boost / CGAL 头文件，把对应目录也加入 `includePath`，例如：
  ```jsonc
  "/home/<user>/OpenFOAM/ThirdParty-v2206/sources/boost/boost_1_74_0",
  "/home/<user>/OpenFOAM/ThirdParty-v2206/sources/cgal/CGAL-4.14.3/include"
  ```

### 3. 自动生成 includePath 目录列表

用一条命令把 src 和 applications 下所有 `lnInclude` 目录列出来，粘进 `includePath`：

```bash
find $FOAM_SRC $FOAM_APP -type d -name lnInclude | sort
```

### 4.（可选）wmake 编译任务

把下面的内容放到用户级任务文件（`~/.config/Code/User/tasks.json`），或工程级 `.vscode/tasks.json`，即可在 VS Code 中直接编译当前文件夹：

```jsonc
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "wmake: 编译当前文件夹",
            "type": "shell",
            "command": "bash -lc 'source /home/<user>/OpenFOAM/OpenFOAM-v2206/etc/bashrc && wmake'",
            "options": { "cwd": "${fileDirname}" },
            "group": "build",
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "wclean: 清理当前文件夹",
            "type": "shell",
            "command": "bash -lc 'source /home/<user>/OpenFOAM/OpenFOAM-v2206/etc/bashrc && wclean'",
            "options": { "cwd": "${fileDirname}" },
            "group": "build"
        }
    ]
}
```

## 方案二：工程级 `c_cpp_properties.json`

在某个工程的 `.vscode/c_cpp_properties.json` 中放置相同配置。工程级配置优先于全局默认值，适合只想对单个工程启用，或全局默认值未能生效时兜底：

```jsonc
{
    "configurations": [
        {
            "name": "OpenFOAM-v2206 (linux64GccDPInt32Opt)",
            "includePath": [
                "${workspaceFolder}/**",
                "/home/<user>/OpenFOAM/OpenFOAM-v2206/src/OpenFOAM/lnInclude",
                "/home/<user>/OpenFOAM/OpenFOAM-v2206/src/finiteVolume/lnInclude",
                "/home/<user>/OpenFOAM/OpenFOAM-v2206/src/meshTools/lnInclude",
                "/home/<user>/OpenFOAM/OpenFOAM-v2206/src/OSspecific/POSIX/lnInclude"
                // ... 其余 lnInclude 目录同上
            ],
            "defines": ["WM_DP", "WM_LABEL_SIZE=32"],
            "compilerPath": "/usr/bin/g++",
            "cStandard": "c11",
            "cppStandard": "c++14",
            "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}
```

## 验证是否生效

1. `Ctrl+Shift+P` → 执行 `Reload Window`（必要时再执行一次 `C/C++: Reset IntelliSense Database`）。
2. 打开任意 `.C` 文件，看右下角语言模式应显示 **C++**（不是纯文本）。
3. `#include "fvCFD.H"` 不再报 `#include errors detected`；按住 `Ctrl` 点击它会跳转到 `src/finiteVolume/cfdTools/general/include/fvCFD.H`。
4. 输入同一个文件里之前定义的变量名前几个字母（或按 `Ctrl+Space`），应出现自动补全。

## 常见问题

- **仍报 `#include errors detected`**：确认 `includePath` 里没有“中间带 `**`”的写法；确认 `lnInclude` 目录存在（OpenFOAM 需先编译过）；改完设置后执行 `Reload Window`。
- **同文件里定义的符号没有提示**：检查右下角语言模式是否为 C++（`.C` / `.H` 的文件关联是否生效）。
- **其它文件里定义的类/函数没有提示**：C++ 必须先用 `#include` 包含对应的 `.H` 头文件才能“看见”这些符号；同时确认 `browse.path` 覆盖了工程目录。
- **宏定义报错（如 `label` 类型不对）**：检查 `defines` 是否与 `$WM_OPTIONS` 匹配（DP/SP、Int32/Int64）。

## 参考：作者本机的实际配置

- OpenFOAM：`/home/chen/OpenFOAM/OpenFOAM-v2206`（`WM_OPTIONS=linux64GccDPInt32Opt`）
- ThirdParty：`/home/chen/OpenFOAM/ThirdParty-v2206`（boost 1.74.0、CGAL 4.14.3）
- 编译器：`/usr/bin/g++`
- 所有配置均放在全局用户设置 `~/.config/Code/User/settings.json` 与 `~/.config/Code/User/tasks.json`，任意 OpenFOAM 工程通用。
