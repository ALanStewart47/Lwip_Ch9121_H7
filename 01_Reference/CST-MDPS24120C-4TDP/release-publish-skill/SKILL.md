---
name: release-publish-skill
description: Use when preparing a CST-MDPS24120C-4TDP release-copy source package, removing implementation comments for delivery, or generating a protected source submission for the company server.
---

# 发布无实现说明代码

使用本 skill 生成可交付副本。正本是唯一持续开发目录；发布副本不可回写到正本。

## 必须遵守

- 仅处理手写代码 `App/`、`Bsp/`、`Protocol/` 内的 `.c/.h`；不手工删除注释。`Core/` 为 CubeMX 生成代码，与 `Drivers/`、`RTT/`、Keil/CubeMX 配置一并逐字节复制，保留全部 CubeMX 注释。块注释内部的 `\` 行续接按注释内容删除；`/*` / `*/` 被行续接切开仍须拒绝。
- 不改动正本，不覆盖已有 `CST-MDPS24120C-4TDP_RELEASE`，不自动 Git 提交、上传或删除目录。
- 排除 `_Doc/`、内部协议文档、根目录 README/STATUS/CHANGELOG 和本 skill；保留驱动、RTT、Keil/CubeMX 配置及其他构建依赖。
- 正本必须先由 Keil 成功构建为零错误；UV4 退出码 0（干净）或 1（仅警告）且日志摘要为 `0 Error(s)` 视为通过。构建失败、找不到 `.hex/.bin`、token 不一致、残留非文件头注释或哈希不一致时，副本均不可发布。
- 构建结果从 UV4 `-o` 日志文件读取，不以 stdout 为准。
- 遇到脚本报告的 `FAILED`、`.RELEASE_FAILED` 或风险提示时，先展示报告并等待用户决定；不得把风险提示描述为已证实缺陷。

## 执行

先确认用户允许在 `Products` 同级创建新目录，然后运行：

```powershell
python .\release-publish-skill\scripts\publish_release.py --keil "C:\Keil_v5\UV4\UV4.exe"
```

若 Keil 不在该默认路径，使用用户机器上的 `UV4.exe` 实际绝对路径替换 `--keil` 参数。脚本会先构建正本，再复制、清理副本、校验 token/哈希，并生成：

```text
../CST-MDPS24120C-4TDP_RELEASE/
../CST-MDPS24120C-4TDP_RELEASE_REPORT.md
```

## 交付判断

只在报告状态为 `SUCCESS` 时，才说“发布副本已验证可提交”。汇报时给出：输出路径、处理文件数、复制的固件产物 SHA-256，以及风险提示数量。

## 禁止替代方案

- 禁止使用正则、编辑器批量替换、`sed` 或手工删除 `//` / `/* */`。
- 禁止为赶时间跳过正本构建、token 验证或报告检查。
- 禁止删除 `Drivers/`、第三方版权/许可证，或把 `_RELEASE` 中失败的副本提交到服务器。

