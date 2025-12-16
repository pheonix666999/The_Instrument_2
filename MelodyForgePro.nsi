!include "MUI2.nsh"
!include "x64.nsh"

Unicode True
RequestExecutionLevel admin
SetCompressor /SOLID lzma

!ifndef INPUT_DIR
  !define INPUT_DIR "dist\\windows"
!endif
!ifndef OUTPUT_FILE
  !define OUTPUT_FILE "dist\\windows\\MelodyForgeProSetup.exe"
!endif

Name "MelodyForge Pro"
OutFile "${OUTPUT_FILE}"
InstallDir "$PROGRAMFILES64\\xAI Music Tools\\MelodyForgePro"

!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_OK "This installer is 64-bit only."
    Abort
  ${EndIf}
FunctionEnd

Section "Install"
  SetOutPath "$INSTDIR"
  File "${INPUT_DIR}\\MelodyForgePro.exe"
  WriteUninstaller "$INSTDIR\\Uninstall.exe"

  ; VST3 install location (64-bit)
  SetOutPath "$COMMONFILES64\\VST3"
  File /r "${INPUT_DIR}\\MelodyForgePro.vst3"

SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\\MelodyForgePro.exe"
  Delete "$INSTDIR\\Uninstall.exe"
  RMDir "$INSTDIR"

  RMDir /r "$COMMONFILES64\\VST3\\MelodyForgePro.vst3"
SectionEnd

