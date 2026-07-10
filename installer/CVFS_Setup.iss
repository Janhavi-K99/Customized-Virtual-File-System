; CVFS File System Explorer — Inno Setup installer script
; Build with: ISCC.exe CVFS_Setup.iss

#define MyAppName "CVFS File System Explorer"
#define MyAppVersion "2.0.0"
#define MyAppPublisher "CVFS Project"
#define MyAppExeName "cvfs_gui.exe"
#define MyAppAssocName "CVFS Virtual Disk"

[Setup]
AppId={{C4F7A3E8-9B2D-4F1E-8A6C-3D5B7E2F1A9C}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\CVFS
DefaultGroupName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=.
OutputBaseFilename=CVFS_Setup

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: checkedonce

[Dirs]
Name: "{app}"; Flags: uninsalwaysuninstaller

[Files]
Source: "{#SourcePath}\..\dist\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourcePath}\..\dist\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourcePath}\..\dist\*.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourcePath}\..\dist\Qt*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourcePath}\..\dist\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs
Source: "{#SourcePath}\..\dist\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs
Source: "{#SourcePath}\..\dist\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs
Source: "{#SourcePath}\..\dist\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs
Source: "{#SourcePath}\..\dist\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs
Source: "{#SourcePath}\..\dist\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs
Source: "{#SourcePath}\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{group}\CLI (cvfs_cli)"; Filename: "{app}\cvfs_cli.exe"; WorkingDir: "{app}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: postinstall nowait skipifsilent unchecked

[Registry]
Root: HKA; Subkey: "Software\Classes\.cvfs\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppAssocName}"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocName}"; ValueType: string; ValueName: ""; ValueData: "CVFS Virtual Disk"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocName}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocName}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""

[Code]
function InitializeSetup: Boolean;
begin
  Result := True;
end;
