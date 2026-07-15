# UEFI Driver 开发作业报告

## 1. 作业题目

### 1.1 搭建 EDK2 环境，并编译 UEFI Driver 小程序

搭建 TianoCore EDK2 开源 UEFI 固件开发环境，基于 UEFI Driver Model 编写一个完整的驱动模块，实现 `EFI_DRIVER_BINDING_PROTOCOL` 的 `Supported()`、`Start()`、`Stop()` 三个标准接口，以及 `ComponentName` 协议用于驱动和设备的名称显示。最终在 QEMU + OVMF 虚拟平台上加载运行，验证驱动模型的标准工作流程。

### 1.2 熟悉 UEFI 启动流程的每个阶段作用

掌握 UEFI 固件从上电到操作系统启动的完整阶段划分，理解每个阶段的职责、内存状态和关键模块，以及各阶段之间的过渡方式。

---

## 2. 思路

### 2.1 UEFI 启动流程

UEFI 固件启动遵循 UEFI PI（Platform Initialization）规范，分为以下五个阶段：

| 阶段 | 全称 | 主要职责 | 内存状态 |
|------|------|----------|----------|
| **SEC** | Security Phase | 安全验证、建立临时内存（Cache-as-RAM）、找到并跳转到 PEI 入口 | 临时内存（CAR） |
| **PEI** | Pre-EFI Initialization | 初始化永久内存（DRAM）、发现系统资源、通过 HOB 传递信息给 DXE | 临时内存 → 永久内存 |
| **DXE** | Driver Execution Environment | 核心服务初始化（协议、事件、内存管理）、调度 DXE 驱动、准备 BDS | 永久内存（全功能） |
| **BDS** | Boot Device Selection | 枚举启动设备、加载启动项、显示启动菜单、启动 OS Loader | 永久内存 + Runtime |
| **RT** | Runtime | `ExitBootServices()` 后，仅有 Runtime Services 可用，最终交出控制权给 OS | 被 OS 接管 |

[插入图片：UEFI 启动阶段流程图]

**各阶段详细说明：**

**① SEC 阶段（Security Phase）**
- 固件复位向量（Reset Vector）第一条指令入口
- 建立临时内存：x86 上通过把 CPU Cache 配置为 Cache-as-RAM（CAR）
- 验证 PEI 阶段代码的完整性（安全启动）
- 找到并跳转到 PEI 入口

**② PEI 阶段（Pre-EFI Initialization）**
- PEI 核心（PEI Core）调度 PEIM（PEI Module）
- 初始化系统内存控制器，发现永久 DRAM
- 移交内存状态（从临时内存切换到永久内存）
- 产生 HOB（Hand-Off Block）传递给 DXE 阶段
- 关键的 PEIM 包括：CPU PEIM、内存发现 PEIM、DXE IPL（DXE Initial Program Loader）

**③ DXE 阶段（Driver Execution Environment）**
- DXE 核心初始化：内存管理、协议服务、事件系统、GCD（Global Coherency Domain）
- DXE 调度器遍历 Firmware Volume 中的 Driver，解析 Depex（Dependency Expression），按依赖关系顺序加载
- 驱动类型包括：DXE_DRIVER（平台初始化驱动）、UEFI_DRIVER（遵循 UEFI Driver Model）
- 加载 PCI、USB、SATA、NVMe、Console、ACPI、SMBIOS 等所有设备驱动
- **我们的 MyDriver 正是在此阶段被加载和注册的**
- 最终发现并执行 BDS 入口

**④ BDS 阶段（Boot Device Selection）**
- 执行启动策略：按 `BootOrder` 遍历启动选项
- 显示启动菜单（如 BIOS 设置界面）
- 加载 Boot Loader（如 Windows Boot Manager、GRUB）
- 调用 `ExitBootServices()` 进入 OS

**⑤ Runtime 阶段**
- `ExitBootServices()` 后，Boot Services 不可用
- 仅有 Runtime Services 保留（如 ResetSystem、GetVariable 等）
- 固件交出控制权给 OS

### 2.2 MyDriver 设计思路

MyDriver 是一个 **UEFI Driver Model** 类型的驱动（`MODULE_TYPE = UEFI_DRIVER`），区别于传统的 DXE_DRIVER，它通过 `EFI_DRIVER_BINDING_PROTOCOL` 实现与设备的动态绑定。

**核心设计模式：**

```
┌─────────────────────────────────────────────────────────┐
│ Entry Point (StudyMyDriverEntryPoint)                    │
│   └─ 安装 DriverBinding + ComponentName 协议到系统       │
│                                                         │
│ DXE 调度器遍历所有 Controller Handle                     │
│   └─ 对每个 Handle 调用 Supported() 探测是否匹配         │
│                                                         │
│ Supported() 成功 → Start()                               │
│   └─ 打开 DevicePath 协议 (BY_DRIVER)                    │
│   └─ 分配 Private Context                                │
│   └─ 安装 Marker Protocol 标识所有权                     │
│                                                         │
│ 需要解绑时 → Stop()                                      │
│   └─ 卸载 Marker Protocol                                │
│   └─ 关闭 DevicePath 协议                                │
│   └─ 释放 Private Context                                │
└─────────────────────────────────────────────────────────┘
```

[插入图片：UEFI Driver Model 工作流程图]

**文件架构：**

```
StudyPkg/
├── StudyPkg.dec          # 包声明文件：定义 GUID、Protocol、PCD
├── StudyPkg.dsc          # 平台描述文件：库映射、模块列表
├── Include/
│   ├── Guid/
│   │   └── StudyDriverGuid.h       # 绑定标记 GUID
│   └── Protocol/
│       └── StudyDriverProtocol.h   # 标记 Protocol 定义
└── MyDriver/
    ├── MyDriver.inf      # 模块信息文件：源文件、依赖、模块类型
    ├── MyDriver.h        # 头文件：私有数据结构、函数声明
    ├── MyDriver.c        # 主实现文件：Supported/Start/Stop
    └── MyDriver.uni      # 模块字符串：名称和描述
```

**关键代码解析：**

**① Entry Point** — 注册驱动到系统 (`MyDriver.c:399-431`)

调用 `EfiLibInstallDriverBindingComponentName2()` 一次性安装三个协议：
- `EFI_DRIVER_BINDING_PROTOCOL` — 驱动绑定核心
- `EFI_COMPONENT_NAME_PROTOCOL` — 传统语言编码的名称
- `EFI_COMPONENT_NAME2_PROTOCOL` — RFC 4646 语言编码的名称

**② Supported()** — 探测能力 (`MyDriver.c:91-131`)

以 `EFI_OPEN_PROTOCOL_BY_DRIVER` 方式打开 `DevicePath` 协议。如果打开成功，说明该控制器支持 DevicePath，立即关闭协议返回成功；否则返回 `EFI_UNSUPPORTED`。

**③ Start()** — 启动控制 (`MyDriver.c:152-236`)

流程：
1. 先用 `TEST_PROTOCOL` 检查是否已经启动，防止重复绑定
2. 分配 `STUDY_MYDRIVER_PRIVATE_DATA` 私有上下文
3. 以 `BY_DRIVER` 方式打开 DevicePath，获取独占访问权
4. 安装标记协议 `gStudyMyDriverProtocolGuid`，标识该控制器归此驱动管理
5. 任何一步失败则通过 `goto` 进行清理

**④ Stop()** — 停止控制 (`MyDriver.c:254-314`)

流程：
1. 通过 `GET_PROTOCOL` 取出私有上下文
2. 卸载标记协议
3. 关闭 DevicePath 协议
4. 释放私有内存

### 2.3 EDK2 构建系统

[插入图片：EDK2 构建系统文件关系图]

EDK2 使用四类元文件控制构建：

| 文件 | 作用 | 在我们的项目中 |
|------|------|----------------|
| `.dec` | 包声明：公开接口、GUID、Protocol、PCD | `StudyPkg/StudyPkg.dec` |
| `.dsc` | 平台描述：库实例映射、模块列表 | `StudyPkg/StudyPkg.dsc` |
| `.inf` | 模块信息：源码、依赖、类型 | `StudyPkg/MyDriver/MyDriver.inf` |
| `.fdf` | Flash 布局（本驱动不需要） | 无 |

---

## 3. 测试

### 1) 编译测试

**环境：** Windows 11 + VS2022 + EDK2

**命令：**

```
build -p StudyPkg/StudyPkg.dsc -m StudyPkg/MyDriver/MyDriver.inf -t VS2022 -a X64 -b DEBUG
```

**结果：**

```
Build start time: 16:04:37, Jul.08 2026
...
Build end time: 16:XX:XX, Jul.08 2026
Build total time: 00:00:XX
```

编译成功，产物为：`Build/StudyPkg/DEBUG_VS2022/X64/MyDriver.efi`

[插入图片：编译成功截图]

### 2) UEFI Shell 加载测试

**环境：** QEMU + OVMF

**步骤：**
1. 将 `MyDriver.efi` 复制到 FAT32 磁盘映像
2. QEMU 启动 OVMF，进入 UEFI Shell
3. 手动加载驱动：`load MyDriver.efi`
4. 通过 `drivers` 命令验证驱动已注册
5. 通过 `dh -d <handle>` 验证绑定详情

**测试命令与结果：**

```
FS0:\> load MyDriver.efi
Image 'FS0:\MyDriver.efi' loaded at 1DF67000 - Success
```

```
FS0:\> drivers
          T D
D         Y C I
R         P F A
U VERSION E G G #D #C DRIVER NAME                            IMAGE NAME
========= ====== = = = == == ================================ ==========
...
9F 00000010 D - - 16 - Study UEFI Driver (MyDriver)          \MyDriver.efi
```

驱动 `9F` 已成功注册，版本号 `00000010`，驱动名 `Study UEFI Driver (MyDriver)`。

[插入图片：drivers 命令输出截图]

### 3) 设备绑定验证

```
FS0:\> dh -d 9F
9F: ComponentName2 ComponentName DriverBinding ImageDevicePath LoadedImage
    Driver Name [9F]      : Study UEFI Driver (MyDriver)
    Driver Image Name     : \MyDriver.efi
    Driver Version        : 00000010
    Driver Type           : Device
    Configuration         : NO
    Diagnostics           : NO
    Managing              :
      Ctrl[03]            : MyDriver-Managed Device
      Ctrl[2C]            : MyDriver-Managed Device
      Ctrl[34]            : MyDriver-Managed Device
      Ctrl[4B]            : MyDriver-Managed Device
      Ctrl[4F]            : MyDriver-Managed Device
      Ctrl[50]            : MyDriver-Managed Device
      Ctrl[55]            : MyDriver-Managed Device
      Ctrl[8C]            : MyDriver-Managed Device
      Ctrl[8F]            : MyDriver-Managed Device
      Ctrl[90]            : QEMU Video PCI Adapter
      Ctrl[92]            : MyDriver-Managed Device
      Ctrl[96]            : MyDriver-Managed Device
      Ctrl[97]            : PS/2 Keyboard Device
      Ctrl[99]            : MyDriver-Managed Device
      Ctrl[9B]            : QEMU QEMU DVD-ROM
      Ctrl[9C]            : FAT File System
```

**验证结论：**

| 测试项 | 结果 |
|--------|------|
| 驱动加载 | ✅ `load MyDriver.efi` 成功，Image 地址 `1DF67000` |
| 协议安装 | ✅ 自动注册了 `ComponentName`、`ComponentName2`、`DriverBinding` |
| 驱动名称 | ✅ 正确显示 "Study UEFI Driver (MyDriver)" |
| Supported 探测 | ✅ 15 个控制器被成功匹配（都有 DevicePath 协议） |
| Start 绑定 | ✅ 15 个控制器进入 "Managing" 列表 |
| 多驱动共存 | ✅ Ctrl[90] 由 QEMU Video Driver 管理，Ctrl[97] 由 PS/2 Driver 管理 |

[插入图片：dh -d 9F 命令输出截图]

---

## 4. 遇到问题

### 4) 构建错误：缺少 `UefiRuntimeServicesTableLib`

**现象：**

```
error 4000: Instance of library class [UefiRuntimeServicesTableLib] is not found
consumed by MdePkg\Library\UefiLib\UefiLib.inf
```

**原因分析：**

`MyDriver` 依赖 `UefiLib`（用于 `EfiLibInstallDriverBindingComponentName2`），而 `UefiLib` 内部依赖 `UefiRuntimeServicesTableLib`。在 `StudyPkg.dsc` 中我们只映射了直接依赖的库，遗漏了这个**间接依赖**。

**解决方案：**

在 `StudyPkg.dsc` 的 `[LibraryClasses]` 中添加：

```
UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
```

**启示：** EDK2 构建系统要求 `.dsc` 文件中必须显式映射所有库依赖，包括传递依赖。可以通过 `build` 报错信息逐条补充，也可以参考同类成熟的 `.dsc`（如 `PcAtChipsetPkg.dsc`）来补全库映射。

### 5) 编译错误：结构体缺少 `Signature` 字段

**现象：**

```
error C2039: "Signature": 不是 "STUDY_MYDRIVER_PRIVATE_DATA" 的成员
```

**原因分析：**

代码中 `Private->Signature = STUDY_MYDRIVER_PRIVATE_DATA_SIGNATURE;` 试图给私有数据结构的 `Signature` 字段赋值，但该结构体定义中并未包含此字段。`Signature` 是 EDK2 私有数据结构的**标准惯用法**，用于运行时类型识别和断言校验。

**解决方案：**

在 `STUDY_MYDRIVER_PRIVATE_DATA` 结构体中添加 `UINTN Signature;` 作为第一个字段：

```c
typedef struct {
  UINTN                             Signature;
  EFI_HANDLE                        Handle;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  EFI_HANDLE                        ControllerHandle;
} STUDY_MYDRIVER_PRIVATE_DATA;
```

**启示：** EDK2 编码规范要求在私有数据结构头部放置 `Signature` 字段，配合 `SIGNATURE_32` 宏和 `CR` 宏使用。这是 EDK2 实现**运行时类型安全**的核心模式。

---

## 5. 导师评价指导

（为空）
