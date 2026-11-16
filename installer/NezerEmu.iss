; Установщик NEZER EMU (Inno Setup 6). Собирается автоматически в GitHub Actions.
; Android Emulator, образ Android и Google Play НЕ входят в установщик: они скачиваются с серверов Google при первом запуске.
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif

[Setup]
AppId={{8C2F6E61-4D7B-4C1E-9A55-1E0D4B7F2A10}
AppName=NEZER EMU
AppVersion={#AppVersion}
AppPublisher=NEZER
DefaultDirName={autopf}\NEZER EMU
DefaultGroupName=NEZER EMU
OutputDir=..\out
OutputBaseFilename=NezerEmuSetup
SetupIconFile=..\assets\nezer.ico
UninstallDisplayIcon={app}\NezerEmu.exe
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
CloseApplications=yes

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "Создать ярлык на рабочем столе"
Name: "whpx"; Description: "Включить «Платформу гипервизора Windows» (ускорение Android, потребуется перезагрузка)"; Flags: unchecked

[Files]
Source: "..\dist\*"; DestDir: "{app}"; Excludes: "vc_redist.x64.exe"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\dist\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Icons]
Name: "{group}\NEZER EMU"; Filename: "{app}\NezerEmu.exe"
Name: "{group}\Удалить NEZER EMU"; Filename: "{uninstallexe}"
Name: "{autodesktop}\NEZER EMU"; Filename: "{app}\NezerEmu.exe"; Tasks: desktopicon

[Run]
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Установка Microsoft Visual C++ Runtime..."; Flags: waituntilterminated
Filename: "{sys}\dism.exe"; Parameters: "/online /enable-feature /featurename:HypervisorPlatform /all /norestart"; StatusMsg: "Включение платформы гипервизора..."; Tasks: whpx; Flags: runhidden waituntilterminated
Filename: "{app}\NezerEmu.exe"; Description: "Запустить NEZER EMU"; Flags: nowait postinstall skipifsilent

[Code]
type
  TMemoryStatusEx = record
    dwLength: DWORD;
    dwMemoryLoad: DWORD;
    ullTotalPhys: Int64;
    ullAvailPhys: Int64;
    ullTotalPageFile: Int64;
    ullAvailPageFile: Int64;
    ullTotalVirtual: Int64;
    ullAvailVirtual: Int64;
    ullAvailExtendedVirtual: Int64;
  end;

function GlobalMemoryStatusEx(var lpBuffer: TMemoryStatusEx): BOOL;
  external 'GlobalMemoryStatusEx@kernel32.dll stdcall';

// Быстрая проверка до установки; полная (CPU, диск, виртуализация) — в мастере при первом запуске.
function InitializeSetup(): Boolean;
var
  m: TMemoryStatusEx;
  mb: Int64;
begin
  Result := True;
  m.dwLength := SizeOf(m);
  if GlobalMemoryStatusEx(m) then
  begin
    mb := m.ullTotalPhys div 1048576;
    if mb < 6000 then
      Result := MsgBox('В компьютере менее 6 ГБ оперативной памяти. Android может работать медленно. Продолжить установку?', mbConfirmation, MB_YESNO) = IDYES;
  end;
end;
