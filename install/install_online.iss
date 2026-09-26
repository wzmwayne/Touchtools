; 脚本由 Inno Setup 脚本向导生成。
; 有关创建 Inno Setup 脚本文件的详细信息，请参阅帮助文档！
; 仅供非商业使用

#define MyAppName "TouchTools"
#define MyAppVersion "1.0"
#define MyAppPublisher "wzmwayne"
#define MyAppURL "wzml.cc.cd"
#define MyAppExeName "TouchtoolsPanel.exe"

#define ProjectRoot "E:\Users\Wayne\Documents\program\c++\Touchtools"
#expr EmitLanguagesSection

[Setup]
; 注意：AppId 的值唯一标识此应用程序。不要在其他应用程序的安装程序中使用相同的 AppId 值。
; (若要生成新的 GUID，请在 IDE 中单击 "工具|生成 GUID"。)
AppId={{1A97FD19-484B-4E3A-A757-40896D2F528C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
; "ArchitecturesAllowed=x64compatible" 指定
; 安装程序只能在 x64 和 Windows 11 on Arm 上运行。
ArchitecturesAllowed=x64compatible
; "ArchitecturesInstallIn64BitMode=x64compatible" 要求
; 在 X64 或 Windows 11 on Arm 上以 "64-位模式" 进行安装，
; 这意味着它应该使用本地 64 位 Program Files 目录
; 和注册表的 64 位视图。
ArchitecturesInstallIn64BitMode=x64compatible
ArchiveExtraction=full
; 使用 "ArchiveExtraction=enhanced"，如果您的所有存档都是 .7z 文件
; 使用 "ArchiveExtraction=enhanced/nopassword"，如果您的所有档案都没有密码保护
DisableProgramGroupPage=yes
LicenseFile={#ProjectRoot}\LICENSE
PrivilegesRequired=lowest
OutputDir={#ProjectRoot}\install
OutputBaseFilename=TouchTools_online
SolidCompression=yes
WizardStyle=modern dynamic

[Files]
Source: "{#ProjectRoot}\dist\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
; 注意：使用 "issigverify" 标志或 "Hash" 参数来验证下载
Source: "https://wzml.cc.cd/Touchtools/dist.7z"; DestDir: "{app}"; DestName: "dist.7z"; ExternalSize: "20866662"; Flags: ignoreversion external download extractarchive recursesubdirs createallsubdirs
; 注意：不要在任何共享系统文件上使用 "Flags: ignoreversion" 

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

