[Setup]
AppId={{A3F1B8C9-2D4E-461A-8B7C-9E0F1A2B3C4D}
AppName=Animated Wallpapers Reimagined
AppVersion=1.0.0.1
AppPublisher=OXOP
DefaultDirName={userappdata}\AnimatedWallpapersReimagined
DefaultGroupName=Animated Wallpapers Reimagined
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputBaseFilename=Animated_Wallpapers_Reimagined_Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64

[Files]
Source: "Animated Wallpapers Reimagined.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "video.mp4"; DestDir: "{app}"; Flags: ignoreversion

[Run]
Filename: "schtasks.exe"; Parameters: "/create /f /tn ""AnimatedWallpapersReimagined"" /tr ""\""{app}\Animated Wallpapers Reimagined.exe\"""" /sc onlogon"; Flags: runhidden
Filename: "{app}\Animated Wallpapers Reimagined.exe"; Description: "{cm:LaunchProgram,Animated Wallpapers Reimagined}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "schtasks.exe"; Parameters: "/delete /f /tn ""AnimatedWallpapersReimagined"""; Flags: runhidden
