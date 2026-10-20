; SupurrSaw by Studio 87, Windows installer (Inno Setup 6). Built by the release workflow.
; Everything is bundled, nothing is downloaded when installing.
#ifndef AppVer
  #define AppVer "0.0.0"
#endif
#ifndef Stage
  #define Stage "stage"
#endif

[Setup]
AppId={{6F0B2A58-5B87-4C1B-9D55-5375727253AB}
AppName=SupurrSaw
AppVersion={#AppVer}
AppPublisher=Studio 87
AppPublisherURL=https://github.com/SpeakingColors/SupurrrSaw-VST
DefaultDirName={autopf}\Studio 87\SupurrSaw
DefaultGroupName=Studio 87
DisableProgramGroupPage=yes
LicenseFile={#Stage}\LICENSE
OutputDir=.
OutputBaseFilename=SupurrSaw_v{#AppVer}_windows_setup
SetupIconFile=icon.ico
UninstallDisplayIcon={app}\SupurrSaw.exe
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
VersionInfoVersion={#AppVer}

[Components]
Name: "vst3"; Description: "SupurrSaw plugin (VST3), installed where your DAW looks for it"; Types: full compact custom; Flags: fixed
Name: "standalone"; Description: "SupurrSaw standalone app (play it without a DAW)"; Types: full custom

[Files]
Source: "{#Stage}\SupurrSaw.vst3\*"; DestDir: "{commoncf64}\VST3\SupurrSaw.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Stage}\SupurrSaw.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "{#Stage}\SupurrSaw_Manual.pdf"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Stage}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\SupurrSaw"; Filename: "{app}\SupurrSaw.exe"; Components: standalone
Name: "{group}\SupurrSaw manual"; Filename: "{app}\SupurrSaw_Manual.pdf"

[Run]
Filename: "{app}\SupurrSaw.exe"; Description: "Start the standalone app now"; Flags: nowait postinstall skipifsilent unchecked; Components: standalone
