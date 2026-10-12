; Inno Setup script for Open Synth (Windows x64).
; Built in CI by:  iscc /DAppVersion=2.1.0 /DArtefacts=<build>\OpenSynth_artefacts\Release /DOutputDir=dist packaging\windows\open-synth.iss
#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef Artefacts
  #define Artefacts "..\..\build\OpenSynth_artefacts\Release"
#endif
#ifndef OutputDir
  #define OutputDir "..\..\dist"
#endif

[Setup]
AppId={{6B1E3C1A-7F3D-4D8E-9C55-0E5A0B7D2F11}
AppName=Open Synth
AppVersion={#AppVersion}
AppPublisher=synth
AppPublisherURL=https://github.com/synthalorian/open-synth
DefaultDirName={autopf}\Open Synth
DefaultGroupName=Open Synth
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputDir={#OutputDir}
OutputBaseFilename=open-synth-{#AppVersion}-windows-x64-setup
SetupIconFile=open-synth.ico
UninstallDisplayIcon={app}\Open Synth.exe
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "standalone"; Description: "Standalone application"; Types: full custom
Name: "vst3"; Description: "VST3 plugin (Common Files\VST3)"; Types: full custom
Name: "clap"; Description: "CLAP plugin (Common Files\CLAP)"; Types: full custom

[Files]
Source: "{#Artefacts}\Standalone\*"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\VST3\Open Synth.vst3\*"; DestDir: "{commoncf64}\VST3\Open Synth.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\CLAP\*"; DestDir: "{commoncf64}\CLAP"; Components: clap; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Open Synth"; Filename: "{app}\Open Synth.exe"; Components: standalone
Name: "{autodesktop}\Open Synth"; Filename: "{app}\Open Synth.exe"; Components: standalone; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Components: standalone; Flags: unchecked

[Run]
Filename: "{app}\Open Synth.exe"; Description: "Launch Open Synth"; Components: standalone; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Open Synth.vst3"
Type: filesandordirs; Name: "{commoncf64}\CLAP\samples"
