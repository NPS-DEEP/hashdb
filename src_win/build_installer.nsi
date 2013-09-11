# NSIS script for creating the Windows hashdb installer file.
#
# Installs the following:
#   32-bit configuration of hashdb tools
#   64-bit configuration of hashdb tools
#   pdf document
#   path
#   uninstaller
#   start menu shortcut for pdf document
#   uninstaller shurtcut
#   registry information including uninstaller information

# Assign VERSION externally with -DVERSION=<ver>
# Build from signed files with -DSIGN
!ifndef VERSION
	!echo "VERSION is required."
	!echo "example usage: makensis -DVERSION=1.0.0 build_installer.nsi"
	!error "Invalid usage"
!endif

!define APPNAME "HashDB ${VERSION}"
!define REG_SUB_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
!define COMPANYNAME "NPS"
!define DESCRIPTION "Hash Database Tools"

# These will be displayed by the "Click here for support information" link in "Add/Remove Programs"
# It is possible to use "mailto:" links here to open the email client
!define HELPURL "//https://github.com/simsong/hashdb" # "Support Information" link
!define UPDATEURL "//https://github.com/simsong/hashdb" # "Product Updates" link
!define ABOUTURL "https://github.com/simsong/hashdb" # "Publisher" link

SetCompressor lzma
 
RequestExecutionLevel admin
 
InstallDir "$PROGRAMFILES\${APPNAME}"
 
Name "${APPNAME}"
	outFile "hashdb-${VERSION}-windowsinstaller.exe"
 
!include LogicLib.nsh
!include EnvVarUpdate.nsi
 
page components
Page instfiles
UninstPage instfiles
 
!macro VerifyUserIsAdmin
UserInfo::GetAccountType
pop $0
${If} $0 != "admin" ;Require admin rights on NT4+
	messageBox mb_iconstop "Administrator rights required!"
	setErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
	quit
${EndIf}
!macroend

function InstallOnce
	# don't install twice
	ifFileExists "$INSTDIR\uninstall.exe" AlreadyThere

	# install Registry information
	WriteRegStr HKLM "${REG_SUB_KEY}" "DisplayName" "${APPNAME} - ${DESCRIPTION}"
	WriteRegStr HKLM "${REG_SUB_KEY}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
	WriteRegStr HKLM "${REG_SUB_KEY}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
	WriteRegStr HKLM "${REG_SUB_KEY}" "InstallLocation" "$\"$INSTDIR$\""
	WriteRegStr HKLM "${REG_SUB_KEY}" "Publisher" "$\"${COMPANYNAME}$\""
	WriteRegStr HKLM "${REG_SUB_KEY}" "HelpLink" "$\"${HELPURL}$\""
	WriteRegStr HKLM "${REG_SUB_KEY}" "URLUpdateInfo" "$\"${UPDATEURL}$\""
	WriteRegStr HKLM "${REG_SUB_KEY}" "URLInfoAbout" "$\"${ABOUTURL}$\""
	WriteRegStr HKLM "${REG_SUB_KEY}" "DisplayVersion" "$\"${VERSION}$\""
	# There is no option for modifying or repairing the install
	WriteRegDWORD HKLM "${REG_SUB_KEY}" "NoModify" 1
	WriteRegDWORD HKLM "${REG_SUB_KEY}" "NoRepair" 1

	# install the uninstaller
	# create the uninstaller
	writeUninstaller "$INSTDIR\uninstall.exe"
 
	# create the start menu for hashdb
	createDirectory "$SMPROGRAMS\${APPNAME}"

	# link the uninstaller to the start menu
	createShortCut "$SMPROGRAMS\${APPNAME}\Uninstall ${APPNAME}.lnk" "$INSTDIR\uninstall.exe"

        # install PDF docs
        setOutPath "$INSTDIR\pdf"
        file "using_hashdb.pdf"

        # 
	createShortCut "$SMPROGRAMS\${APPNAME}\hashdb Manual.lnk" "$INSTDIR\pdf\using_hashdb.pdf"

	AlreadyThere:
functionEnd

function .onInit
	setShellVarContext all
	!insertmacro VerifyUserIsAdmin
functionEnd

Section "32-bit configuration"

	# install content common to both
	call InstallOnce

	# install hashdb files into the 32-bit configuration
	setOutPath "$INSTDIR\32-bit"
	file "/oname=hashdb_manager.exe" "hashdb_manager32.exe"
	file "/oname=hashdb_checker.exe" "hashdb_checker32.exe"
sectionEnd

Section "64-bit configuration"

	# install content common to both
	call InstallOnce

	# install hashdb files into the 64-bit configuration
	setOutPath "$INSTDIR\64-bit"
	file "/oname=hashdb_manager.exe" "hashdb_manager64.exe"
	file "/oname=hashdb_checker.exe" "hashdb_checker64.exe"
sectionEnd

Section "Add to path"
	setOutPath "$INSTDIR"
        # note that path includes 32-bit and 64-bit, whether or not they
        # were both installed
        ${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$INSTDIR\32-bit"
        ${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$INSTDIR\64-bit"
sectionEnd

function un.onInit
	SetShellVarContext all
 
	#Verify the uninstaller - last chance to back out
	MessageBox MB_OKCANCEL "Permanantly remove ${APPNAME}?" IDOK next
		Abort
	next:
	!insertmacro VerifyUserIsAdmin
functionEnd
 
Function un.FailableDelete
	Start:
	delete "$0"
	IfFileExists "$0" FileStillPresent Continue

	FileStillPresent:
	DetailPrint "Unable to delete file $0, likely because it is in use.  Please close all hashdb files and try again."
	MessageBox MB_ICONQUESTION|MB_RETRYCANCEL \
		"Unable to delete file $0, \
		likely because it is in use.  \
		Please close all Bulk Extractor files and try again." \
 		/SD IDABORT IDRETRY Start IDABORT InstDirAbort

	# abort
	InstDirAbort:
	DetailPrint "Uninstall started but did not complete."
	Abort

	# continue
	Continue:
FunctionEnd

section "uninstall"
	# manage uninstalling these because they may be open
	StrCpy $0 "$INSTDIR\32-bit\hashdb_manager.exe"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\32-bit\hashdb_checker.exe"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\64-bit\hashdb_manager.exe"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\64-bit\hashdb_checker.exe"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\pdf\using_hashdb.pdf"
	Call un.FailableDelete

	# uninstall files and links
	delete "$INSTDIR\32-bit\*"
	delete "$INSTDIR\64-bit\*"
	delete "$INSTDIR\pdf\*"

	# uninstall dir
	rmdir "$INSTDIR\32-bit"
	rmdir "$INSTDIR\64-bit"
	rmdir "$INSTDIR\pdf"

	# uninstall Start Menu launcher shortcuts
	delete "$SMPROGRAMS\${APPNAME}\hashdb Manual.lnk"
	delete "$SMPROGRAMS\${APPNAME}\uninstall ${APPNAME}.lnk"
	rmDir "$SMPROGRAMS\${APPNAME}"

	# delete the uninstaller
	delete "$INSTDIR\uninstall.exe"
 
	# Try to remove the install directory
	rmDir "$INSTDIR"
 
	# Remove uninstaller information from the registry
	DeleteRegKey HKLM "${REG_SUB_KEY}"

        # remove associated search paths from the PATH environment variable
        # were both installed
        ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "$INSTDIR\32-bit"
        ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "$INSTDIR\64-bit"
sectionEnd

